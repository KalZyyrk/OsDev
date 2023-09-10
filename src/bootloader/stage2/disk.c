#include "disk.h"
#include "x86.h"

bool DISK_Initialize(DISK* disk, uint8_t driveNumber)
{
  uint8_t driveType;
  uint16_t cylinders, sectors, heads;

  if (!x86_Disk_GetDriveParams(disk->id, &driveType, &cylinders, &sectors, &heads))
    return false;

  disk->id = driveNumber;
  disk->cylinders = cylinders;
  disk->heads = heads;
  disk->sectors = sectors;
  
  return true;
}

void DISK_LBA2CHS(DISK* disk, uint32_t lba, uint16_t* cylinderOut, uint16_t* sectorsOut, uint16_t* headsOut)
{
  // sector = (LBA % sector per track + 1)
  *sectorsOut = lba % disk->sectors + 1;

  // cylinder = (LBA / sectors per track) / heads
  *cylinderOut = (lba/ disk->sectors) / disk->heads;

  // head = (LBA / sectors per track) % heads
  *headsOut = (lba/ disk->sectors) % disk->heads;
}

bool DISK_ReadSectors(DISK* disk, uint32_t lba, uint8_t sectors, void far* dataOut)
{
  uint16_t cylinder, sector, head;

  DISK_LBA2CHS(disk, lba, &cylinder, &sector, &head);

  for (int i = 0; i < 3; i++)
  {
    if (x86_Disk_Read(disk->id, cylinder, sector, head, sectors, dataOut))
      return true;
    
    x86_Disk_Reset(disk->id);
  }

  return false;
}
