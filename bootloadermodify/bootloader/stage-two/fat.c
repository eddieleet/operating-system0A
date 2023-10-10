#include "fat.h"
#include "stdio.h"
#include "memdefs.h "
#include " string.h"
#include "memory.h "
#include "ctype.h "
#include <sttddef.h>
#include "minmax.h "
#include "stdlib.h "


#define SECTOR_SIZE  512 
#define MAX_PATH_SIZE  256
#define MAX_FILE_HANDLES 10 
#define ROOT_DIRECTORY_HANDLE -1
#define FAT_CACHE_SIZE 5


typedef struct {
    // extended booot record 
    uint8_t DriveNumber;
    uint8_t _Reserved;
    uint8_t Signature;
    uint32_t VolumeId; // serial number , value doesn't matter
    uint8_t VolumeLabel[11]; // 11 bytes, padded with spaces
    uint8_t SystemId[8];
}__attribute__((packed)) FAT_ExtendedBootRecord;


typedef struct {
    uint32_t SectorsPerFat;
    uint16_t Flags;
    uint16_t FatVersion;
    uint32_t RootDirectoryCluster;
    uint16_t FSInfoSector;
    uint16_t BackupBootSector;
    uint8_t _Reserved[12];
    FAT_ExtendedBootRecord EBR;
}__attribute((packed)) FAT32_ExtendedBootRecord;


typedef struct {
    uint8_t BootJumpInstruction[3];
    uint8_t OemIdentifier[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FatCount;
    uint16_t DirEntryCount;
    uint16_t TotalSectors;
    uint8_t MediaDescriptorType;
    uint16_t SectorsPerFat;
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t LargeSectorCount;


    union {
        FAT _ExtendedBootRecord EBR1216;
        FAT32_ExtendedBootRecord EBR32;
    }
// ... we don't care about code ....
} __attribute__((packed)) FAT_BootSector;


typedef struct{
    uint8_t Buffer[SECTOR_SIZE];
    FAT_File Public;
    bool Opened;
    uint32_t FirstCluster;
    uint32_t CurrentCluster;
    uint32_t CurrentSectorInCluster;
} FAT_FileData;

typedef struct {
    uint8_t Order;
    int16_t Chars[13];

} FAT_LFNBlock;


typedef struct {
    union{
        FAT_BootSector BootSector;
        uint8_t BootSectorBytes[SECTOR_SIZE];
    } BS;

    FAT_FileData RootDirectory;

    FAT_FileData OpenedFiles[MAX_FILE_HANDLES];

    uint8_t FatCache[FAT_CACHE_SIZE * SECTOR_SIZE];
    uint32_t FatCachePosition;

    FAT_LFNBlock FAT_LFNBlock[FAT_LFN_LAST];
    int LFNCount;
} FAT_Data;

static FAT_Data* g_Data;
static uint32_t g_DataSectionLba;
static uint8_t g_FatType;
static uint32_t g_TotalSectors;
static uint32_t g_SectorsPerFat;


uint32_t FAT_ClusterToLba(uint32_t cluster);

int FAT_CompareLFNBlocks(const void* blockA, const void* blockB){
    FAT_LFNBlock* a = (FAT_LFNBlock*)blockA;
    FAT_LFNBlock* b = (FAT_LFNBlock*)blockB;
    return ((int)a->Order) - ((int)b->Order);
}

bool FAT_ReadBootSector(Partition* disk){
    return Partition_ReadSectors(disk, 0,1, g_Data->BS.BootSectorBytes)

}

bool FAT_ReadFat(Partition* disk, size_t lbalndex){
    return Partition_ReadSectors(disk, g_Data->BS.BootSector.ReservedSectors + lbalndex, FAT_CACHE_SIZE, g_Data->FatCache);
}

void FAT_Detect(Partition* disk){
    uint32_t dataClusters = (g_TotalSectors - g_DataSectionLba)
    if (dataClusters < 0xFF5)
      g_FatType = 12;
      else if (g_Data->BS.BootSector.SectorsPerFat != 0)
       g_FatType = 16;
    else g_FatType = 32;

}

bool FAT_Initialize(Partition* disk)
{
    g_Data = (FAT_Data* ) MEMORY_FAT_ADDR;

    //read boot sector
    if (!FAT_ReadBootSector(disk)){
        printf("FAT: read boot sector failed\r\n");
        return false;
    }

    // read FAT
    g_Data->FatCachePosition = 0xFFFFFFFF;

    g_TotalSectors = g_Data->BS.BootSector.TotalSectors;
    if(g_TotalSectors == 0){ // fat32
  g_TotalSectors = g_Data->BS.BootSector.LargeSectorCount;
    }


    bool isFat32 = false;
    g_sectorsPerFat = g_Data->BS.BootSector.SectorsPerFat;
    if(g_SectorsPerFat == 0){  // fat32
     isFat32 = true;
     g_SectorsPerFat = g_Data->BS.BootSector.EBR32.SectorsPerFat;
    }


    // open root directory file 
    uint32_t rootDirLba;
    uint32_t rootDirSize;
    if(isFat32){
        g_DataSectionLba = g_Data->BS.BootSector.ReservedSectors + g_SectorsPerFat * g_Data->BS.BootSector.FatCount;
        rootDirLba = FAT_ClusterToLba(g_Data->BS.BootSector.EBR32.RootDirectoryCluster);
        rootDirSize = 0;
    }
    else{
        rootDirLba = g_Data->BS.BootSector.ReservedSectors + g_SectorsPerFat * g_Data->BS.BootSector.FatCount;
        rootDirSize = sizeof(FAT_DirectoryEntry) * g_Data->BS.BootSector.DirEntryCount;
        uint32_t rootDirSectors = (rootDirSize + g_Data->BS.BootSector.BytesPerSector - 1) / g_Data->BS.BootSector.BytesPerSector;
        g_DataSectionLba = rootDirLba + rootDirSectors;

    }

g_Data->RootDirectory.Public.Handle = ROOT_DIRECTORY_HANDLE;
g_Data->RootDirectory.Public.IsDirectory = true;
g_Data->RootDirectory.Public.Position = 0;
g_Data->RootDirectory.Public.Size = sizeof(FAT_DirectoryEntry) * g_Data->BS.BootSector.DirEntryCount;
g_Data->RootDirectory.Opened = true;
g_Data->RootDirectory.FirstCluster= rootDirLba;
g_Data->RootDirectory.CurrentCluster = rootDirLba;
g_Data->g_Data->RootDirectory.CurrentSectorInCluster = 0;

if(!Partition_ReadSectors(disk, rootDirLba, 1, g_Data->RootDirectory.Buffer)){
    printf("FAT: read root directory failed\r\n");
    return false;
}

// calculate data section
FAT_Detect(disk);

// reset opened files
for(int i = 0; i < MAX_FILE_HANDLES; i++)
g_Data->OpenedFiels[i].Opened = false;
g_Data->LFNCount = 0;

return true;

}


uint32_t FAT_ClusterToLba(uint32_t cluster){
    return g_DataSectionLba + (cluster - 2 ) * g_Data->BS.BootSector.SectorsPerCluster;
}

FAT_File* FAT_OpenEntry(Partitiion* disk, FAT_DirectoryEntry*  entry){

    // find empty handle
    int handle = -1 ;
    for(int i =0; i < MAX_FILE_HANDLE && handle < 0; i++){
        if (!g_Data->OpenedFiles[i].Opened)
        handle = i;
    }

    // out of handles 
    if (handle < 0 )
    {
        printf("FAT: out of file handles\r\n");
        return false;
    }

    // setup vars
    FAT_FileData* fd = &g_Data->OpenedFiles[handle];
    fd->Public.Handle = handle;
    fd->Public.IsDirectory = (entry->Attributes & FAT_ATTRIBUTE_DIRECTORY) !=-0;
    fd->Public.Position = 0;
    fd->Public.Size = entry->Size;
    fd->
}