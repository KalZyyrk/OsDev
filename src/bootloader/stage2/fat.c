#include "fat.h"
#include "memdefs.h"
#include "utility.h"
#include "stdio.h"

#define SECTOR_SIZE   512

#pragma pack(push, 1)

typedef struct 
{
  uint8_t BootJumpInstruction[3];
  uint8_t OemIdentifier[8];
  uint16_t BytesPerSector;
  uint8_t SectorPerCluster;
  uint16_t ReservedSectors;
  uint8_t FatCount;
  uint16_t DirEntryCount;
  uint16_t TotalSectors;
  int8_t MediaDescription;
  int16_t SectorsPerFat;
  int16_t SectorsPerTrack;
  int16_t Heads;
  int32_t HiddenSectors;
  int32_t LargeSectorCount;

  // extended boot record
  int8_t DriveNumber;
  int8_t _Reserved;
  int8_t Signature;
  int32_t VolumeId;
  int8_t VolumeLabel[11];
  int8_t SystemId[8];

  // ... we don't care about code ...

} FAT_BootSector;

#pragma pack(pop)

typedef struct
{
  union
  {
    FAT_BootSector BootSector;
    uint8_t BootSectorBytes[SECTOR_SIZE];
  } BS;
} FAT_Data;

static FAT_Data far* g_Data;
static uint8_t far* g_Fat = NULL;
static FAT_DirectoryEntry far* g_RootDirectory = NULL;
static uint32_t g_RootDirectoryEnd;

bool FAT_ReadBootSector(DISK* disk)
{
  return DISK_ReadSectors(disk, 0, 1, g_Data->BS.BootSectorBytes);
}

bool FAT_ReadFat(DISK* disk)
{
  return FAT_ReadSectors(disk, g_Data->BS.BootSector.ReservedSectors, g_Data->BS.BootSector.SectorsPerFat, g_Fat);
}

bool FAT_ReadRootDirectory(DISK* disk)
{
  uint32_t lba = g_Data->BS.BootSector.ReservedSectors + g_Data->BS.BootSector.SectorsPerFat * g_Data->BS.BootSector.FatCount;
  uint32_t size = sizeof(DirectoryEntry) * g_Data->BS.BootSector.DirEntryCount;
  uint32_t sectors = (size + g_Data->BS.BootSector.BytesPerSector - 1) / g_Data->BS.BootSector.BytesPerSector;
  
  g_RootDirectoryEnd = lba + sectors;
  return DISK_ReadSectors(disk, lba, sectors, g_RootDirectory);
}

bool FAT_Initialize(DISK* disk)
{
  // read boot sector
  if (!FAT_ReadBootSector(disk))
  {
    g_Data = (FAT_Data far*)MEMORY_FAT_ADDR;

    printf("FAT: read boot sector failed\r\n");
    return false;
  }
  
  // read FAT
  g_Fat = (uint8_t far*)g_Data + sizeof(FAT_Data);   
  uint32_t fatSize = g_Data->BS.BootSector.BytesPerSector * g_Data->BS.BootSector.SectorsPerFat;
  if (sizeof(FAT_Data) + fatSize >= MEMORY_FAT_SIZE)
  {
    printf("FAT: not enough memory to read FAT! Required %lu, only have %u\r\n", sizeof(FAT_Data) + fatSize, MEMORY_FAT_SIZE);
    return false;
  }

  if (!FAT_ReadFat(disk))
  {
    printf("FAT: read FAT failed\r\n");
    return false;
  }

  // read root directory
  g_RootDirectory = (FAT_DirectoryEntry far*)(g_Fat + fatSize);
  uint32_t rootDirSize = sizeof(FAT_DirectoryEntry) * g_Data->BS.BootSector.DirEntryCount;
  rootDirSize = align(rootDirSize, g_Data->BS.BootSector.BytesPerSector);

  if (sizeof(FAT_Data) + fatSize + rootDirSize >= MEMORY_FAT_SIZE)
  {
    printf("FAT: not enough memory to read root directory! Required %lu, only have %u\r\n", sizeof(FAT_Data) + fatSize + rootDirSize, MEMORY_FAT_SIZE);
    return false;
  }

  if (!FAT_ReadRootDirectory(disk))
  {
    printf("FAT: read FAT failed\r\n");
    return false;
  }

}

bool readSectors(FILE* disk, uint32_t lba, uint32_t count, void* bufferOut) 
{
  bool ok = true;
  ok = ok && (fseek(disk, lba * g_BootSector.BytesPerSector, SEEK_SET) == 0);
  ok = ok && (fread(bufferOut, g_BootSector.BytesPerSector, count, disk) == count);
  return ok;
}



DirectoryEntry* findFile(const char* name)
{
  for (uint32_t i = 0; i < g_BootSector.DirEntryCount; i++) 
  {
    if (memcmp(name, g_RootDirectory[i].Name, 11) == 0) return &g_RootDirectory[i];
  }

  return NULL;
}

bool readFile(DirectoryEntry* fileEntry, FILE* disk, uint8_t* outputBuffer)
{
  bool ok = true;
  uint16_t currentCluster = fileEntry->FirstClusterLow;

  do {
    uint32_t lba = g_RootDirectoryEnd + (currentCluster - 2) * g_BootSector.SectorPerCluster;
    ok = ok && readSectors(disk, lba, g_BootSector.SectorPerCluster, outputBuffer);
    outputBuffer += g_BootSector.SectorPerCluster * g_BootSector.BytesPerSector;

    uint32_t fatIndex = currentCluster * 3 / 2;
    if (currentCluster % 2 == 0) currentCluster = (*(uint16_t*)(g_Fat + fatIndex)) & 0x0FFF;
    else currentCluster = (*(uint16_t*)(g_Fat + fatIndex)) >> 4;

  } while (ok && currentCluster < 0x0FF8);
  
  return ok;
}

int main (int argc, char** argv) 
{
  if (argc < 3) {
    printf("Syntax: %s <disk image> <file name> \n", argv[0]);
    return -1;
  }

  FILE* disk = fopen(argv[1], "rb");
  if (!disk)
  {
    fprintf(stderr, "Cannot open disk image %s!", argv[1]);
    return -1;
  }
  
  if (!readBootSector(disk))
  {
    fprintf(stderr, "Could not read boot sector!\n");
    return -2;
  }

  if (!readFat(disk))
  {
    fprintf(stderr, "Could not read FAT!\n");
    free(g_Fat);
    return -3;
  }

  if (!readRootDirectory(disk))
  {
    fprintf(stderr, "Could not read FAT!\n");
    free(g_Fat);
    free(g_RootDirectory);
    return -4;
  }

  DirectoryEntry* fileEntry = findFile(argv[2]);
  if (!fileEntry) 
  {
    fprintf(stderr, "Could not find file %s!\n", argv[2]);
    free(g_Fat);
    free(g_RootDirectory);
    return -5;
  }

  uint8_t* buffer = (uint8_t*) malloc(fileEntry->Size + g_BootSector.BytesPerSector);
  if (!readFile(fileEntry, disk, buffer))
  {
    fprintf(stderr, "Could not find file %s!\n", argv[2]);
    free(g_Fat);
    free(g_RootDirectory);
    free(buffer);
    return -5;
  }

  for (size_t i = 0; i < fileEntry->Size; i++)
  {
    if (isprint(buffer[i])) fputc(buffer[i], stdout);
    else printf("<%02x>", buffer[i]);
  }
  printf("\n");

  free(g_Fat);
  free(g_RootDirectory);
  return 0;
}