/** @file
  Copyright (c) 2022, Mikhail Krichanov. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/
#include <Ext4Dxe.h>

#include <UserFile.h>
#include <UserGlobalVar.h>
#include <UserMemory.h>
#include <string.h>

#define OPEN_FILE_MODES_COUNT  3
#define MAP_TABLE_SIZE         0x100
#define CHAR_FAT_VALID         0x01

#define TO_UPPER(a)  (CHAR16) ((a) <= 0xFF ? mEngUpperMap[a] : (a))

UINT8  _gPcd_FixedAtBuild_PcdUefiVariableDefaultLang[4]         = { 101, 110, 103, 0 };
UINT8  _gPcd_FixedAtBuild_PcdUefiVariableDefaultPlatformLang[6] = { 101, 110, 45, 85, 83, 0 };

STATIC UINTN        mFuzzOffset;
STATIC UINTN        mFuzzSize;
STATIC CONST UINT8  *mFuzzPointer;

STATIC EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *mEfiSfsInterface;

STATIC UINT64  mOpenFileModes[OPEN_FILE_MODES_COUNT] = { EFI_FILE_MODE_READ, EFI_FILE_MODE_WRITE, EFI_FILE_MODE_CREATE };

CHAR8  mEngUpperMap[MAP_TABLE_SIZE];
CHAR8  mEngLowerMap[MAP_TABLE_SIZE];
CHAR8  mEngInfoMap[MAP_TABLE_SIZE];

CHAR8  mOtherChars[] = {
  '0',
  '1',
  '2',
  '3',
  '4',
  '5',
  '6',
  '7',
  '8',
  '9',
  '\\',
  '.',
  '_',
  '^',
  '$',
  '~',
  '!',
  '#',
  '%',
  '&',
  '-',
  '{',
  '}',
  '(',
  ')',
  '@',
  '`',
  '\'',
  '\0'
};

VOID
UnicodeCollationInitializeMappingTables (
  VOID
  )
{
  UINTN  Index;
  UINTN  Index2;

  //
  // Initialize mapping tables for the supported languages
  //
  for (Index = 0; Index < MAP_TABLE_SIZE; Index++) {
    mEngUpperMap[Index] = (CHAR8)Index;
    mEngLowerMap[Index] = (CHAR8)Index;
    mEngInfoMap[Index]  = 0;

    if (((Index >= 'a') && (Index <= 'z')) || ((Index >= 0xe0) && (Index <= 0xf6)) || ((Index >= 0xf8) && (Index <= 0xfe))) {
      Index2               = Index - 0x20;
      mEngUpperMap[Index]  = (CHAR8)Index2;
      mEngLowerMap[Index2] = (CHAR8)Index;

      mEngInfoMap[Index]  |= CHAR_FAT_VALID;
      mEngInfoMap[Index2] |= CHAR_FAT_VALID;
    }
  }

  for (Index = 0; mOtherChars[Index] != 0; Index++) {
    Index2               = mOtherChars[Index];
    mEngInfoMap[Index2] |= CHAR_FAT_VALID;
  }
}

/**
  Performs a case-insensitive comparison of two Null-terminated strings.

  @param  This Protocol instance pointer.
  @param  Str1 A pointer to a Null-terminated string.
  @param  Str2 A pointer to a Null-terminated string.

  @retval 0   Str1 is equivalent to Str2
  @retval > 0 Str1 is lexically greater than Str2
  @retval < 0 Str1 is lexically less than Str2

**/
INTN
StriColl (
  IN CHAR16  *Str1,
  IN CHAR16  *Str2
  )
{
  while (*Str1 != 0) {
    if (TO_UPPER (*Str1) != TO_UPPER (*Str2)) {
      break;
    }

    Str1 += 1;
    Str2 += 1;
  }

  return TO_UPPER (*Str1) - TO_UPPER (*Str2);
}

/**
   Initialises Unicode collation, which is needed for case-insensitive string comparisons
   within the driver (a good example of an application of this is filename comparison).

   @param[in]      DriverHandle    Handle to the driver image.

   @retval EFI_SUCCESS   Unicode collation was successfully initialised.
   @retval !EFI_SUCCESS  Failure.
**/
EFI_STATUS
Ext4InitialiseUnicodeCollation (
  EFI_HANDLE  DriverHandle
  )
{
  UnicodeCollationInitializeMappingTables ();
  return EFI_SUCCESS;
}

/**
   Does a case-insensitive string comparison. Refer to EFI_UNICODE_COLLATION_PROTOCOL's StriColl
   for more details.

   @param[in]      Str1   Pointer to a null terminated string.
   @param[in]      Str2   Pointer to a null terminated string.

   @retval 0   Str1 is equivalent to Str2.
   @retval >0  Str1 is lexically greater than Str2.
   @retval <0  Str1 is lexically less than Str2.
**/
INTN
Ext4StrCmpInsensitive (
  IN CHAR16  *Str1,
  IN CHAR16  *Str2
  )
{
  return StriColl (Str1, Str2);
}

EFI_STATUS
EFIAPI
EfiLibInstallAllDriverProtocols2 (
  IN CONST EFI_HANDLE ImageHandle,
  IN CONST EFI_SYSTEM_TABLE *SystemTable,
  IN EFI_DRIVER_BINDING_PROTOCOL *DriverBinding,
  IN EFI_HANDLE DriverBindingHandle,
  IN CONST EFI_COMPONENT_NAME_PROTOCOL *ComponentName, OPTIONAL
  IN CONST EFI_COMPONENT_NAME2_PROTOCOL       *ComponentName2, OPTIONAL
  IN CONST EFI_DRIVER_CONFIGURATION_PROTOCOL  *DriverConfiguration, OPTIONAL
  IN CONST EFI_DRIVER_CONFIGURATION2_PROTOCOL *DriverConfiguration2, OPTIONAL
  IN CONST EFI_DRIVER_DIAGNOSTICS_PROTOCOL    *DriverDiagnostics, OPTIONAL
  IN CONST EFI_DRIVER_DIAGNOSTICS2_PROTOCOL   *DriverDiagnostics2    OPTIONAL
  )
{
  abort ();
  return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
EfiLibUninstallAllDriverProtocols2 (
  IN EFI_DRIVER_BINDING_PROTOCOL *DriverBinding,
  IN CONST EFI_COMPONENT_NAME_PROTOCOL *ComponentName, OPTIONAL
  IN CONST EFI_COMPONENT_NAME2_PROTOCOL       *ComponentName2, OPTIONAL
  IN CONST EFI_DRIVER_CONFIGURATION_PROTOCOL  *DriverConfiguration, OPTIONAL
  IN CONST EFI_DRIVER_CONFIGURATION2_PROTOCOL *DriverConfiguration2, OPTIONAL
  IN CONST EFI_DRIVER_DIAGNOSTICS_PROTOCOL    *DriverDiagnostics, OPTIONAL
  IN CONST EFI_DRIVER_DIAGNOSTICS2_PROTOCOL   *DriverDiagnostics2    OPTIONAL
  )
{
  abort ();
  return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
WrapInstallMultipleProtocolInterfaces (
  IN OUT EFI_HANDLE  *Handle,
  ...
  )
{
  VA_LIST     Args;
  EFI_STATUS  Status;
  EFI_GUID    *Protocol;
  VOID        *Interface;

  if (Handle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  VA_START (Args, Handle);
  for (Status = EFI_SUCCESS; !EFI_ERROR (Status);) {
    //
    // If protocol is NULL, then it's the end of the list
    //
    Protocol = VA_ARG (Args, EFI_GUID *);
    if (Protocol == NULL) {
      break;
    }

    Interface = VA_ARG (Args, VOID *);

    //
    // If this is Sfs protocol then save interface into global state
    //
    if (CompareGuid (Protocol, &gEfiSimpleFileSystemProtocolGuid)) {
      mEfiSfsInterface = Interface;
    }
  }

  VA_END (Args);

  return Status;
}

VOID
FreeAll (
  IN CHAR16          *FileName,
  IN EXT4_PARTITION  *Part
  )
{
  FreePool (FileName);

  if (Part != NULL) {
    if (Part->DiskIo != NULL) {
      FreePool (Part->DiskIo);
    }

    if (Part->BlockIo != NULL) {
      if (Part->BlockIo->Media != NULL) {
        FreePool (Part->BlockIo->Media);
      }

      FreePool (Part->BlockIo);
    }

    if (Part->Root != NULL) {
      Ext4UnmountAndFreePartition (Part);
    } else if (Part != NULL) {
      FreePool (Part);
    }
  }
}

EFI_STATUS
EFIAPI
FuzzReadDisk (
  IN  EFI_DISK_IO_PROTOCOL  *This,
  IN  UINT32                MediaId,
  IN  UINT64                Offset,
  IN  UINTN                 BufferSize,
  OUT VOID                  *Buffer
  )
{
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((mFuzzSize - mFuzzOffset) < BufferSize) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (Buffer, mFuzzPointer, BufferSize);
  mFuzzPointer += BufferSize;
  mFuzzOffset  += BufferSize;

  return EFI_SUCCESS;
}

STATIC
VOID
ConfigureMemoryAllocations (
  IN  CONST UINT8  *Data,
  IN  UINTN        Size
  )
{
  UINT32  Off;

  mPoolAllocationIndex = 0;
  mPageAllocationIndex = 0;

  //
  // Limit single pool allocation size to 3GB
  //
  SetPoolAllocationSizeLimit (BASE_1GB | BASE_2GB);

  Off = sizeof (UINT64);
  if (Size >= Off) {
    CopyMem (&mPoolAllocationMask, &Data[Size - Off], sizeof (UINT64));
  } else {
    mPoolAllocationMask = MAX_UINT64;
  }

  Off += sizeof (UINT64);
  if (Size >= Off) {
    CopyMem (&mPageAllocationMask, &Data[Size - Off], sizeof (UINT64));
  } else {
    mPageAllocationMask = MAX_UINT64;
  }
}

STATIC
INT32
TestExt4Dxe (
  CONST UINT8  *FuzzData,
  UINTN        FuzzSize
  )
{
  EFI_STATUS             Status;
  EXT4_PARTITION         *Part;
  EXT4_FILE              *File;
  EFI_FILE_PROTOCOL      *This;
  EFI_DISK_IO_PROTOCOL   *DiskIo;
  EFI_BLOCK_IO_PROTOCOL  *BlockIo;
  EFI_HANDLE             DeviceHandle;
  UINTN                  BufferSize;
  VOID                   *Buffer;
  EFI_FILE_PROTOCOL      *NewHandle;
  CHAR16                 *FileName;
  VOID                   *Info;
  UINTN                  Len;
  UINT64                 Position;
  UINT64                 FileSize;
  UINTN                  Index;

  Part       = NULL;
  BufferSize = 100;

  mFuzzOffset      = 0;
  mFuzzSize        = FuzzSize;
  mFuzzPointer     = FuzzData;
  mEfiSfsInterface = NULL;

  DeviceHandle = (EFI_HANDLE)0xDEADBEAFULL;

  //
  // Construct file name
  //
  FileName = AllocateZeroPool (BufferSize);
  if (FileName == NULL) {
    return 0;
  }

  ASAN_CHECK_MEMORY_REGION (FileName, BufferSize);

  if ((mFuzzSize - mFuzzOffset) < BufferSize) {
    FreePool (FileName);
    return 0;
  }

  CopyMem (FileName, mFuzzPointer, BufferSize - 2);
  mFuzzPointer += BufferSize - 2;
  mFuzzOffset  += BufferSize - 2;

  //
  // Construct BlockIo and DiskIo interfaces
  //
  DiskIo = AllocateZeroPool (sizeof (EFI_DISK_IO_PROTOCOL));
  if (DiskIo == NULL) {
    FreePool (FileName);
    return 0;
  }

  ASAN_CHECK_MEMORY_REGION (DiskIo, sizeof (EFI_DISK_IO_PROTOCOL));

  DiskIo->ReadDisk = FuzzReadDisk;

  BlockIo = AllocateZeroPool (sizeof (EFI_BLOCK_IO_PROTOCOL));
  if (BlockIo == NULL) {
    FreePool (FileName);
    FreePool (DiskIo);
    return 0;
  }

  ASAN_CHECK_MEMORY_REGION (BlockIo, sizeof (EFI_BLOCK_IO_PROTOCOL));

  BlockIo->Media = AllocateZeroPool (sizeof (EFI_BLOCK_IO_MEDIA));
  if (BlockIo->Media == NULL) {
    FreePool (FileName);
    FreePool (DiskIo);
    FreePool (BlockIo);
    return 0;
  }

  ASAN_CHECK_MEMORY_REGION (BlockIo->Media, sizeof (EFI_BLOCK_IO_MEDIA));

  //
  // Check Ext4 SuperBlock magic like it done
  // in Ext4IsBindingSupported routine
  //
  if (!Ext4SuperblockCheckMagic (DiskIo, BlockIo)) {
    // Don't halt on bad magic, just keep going
    DEBUG ((DEBUG_WARN, "[ext4] Superblock contains bad magic \n"));
  }

  //
  // Open partition
  //
  Status = Ext4OpenPartition (DeviceHandle, DiskIo, NULL, BlockIo);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[ext4] Error mounting: %r\n", Status));
    FreePool (FileName);
    FreePool (BlockIo->Media);
    FreePool (BlockIo);
    FreePool (DiskIo);
    return 0;
  }

  Part = (EXT4_PARTITION *)mEfiSfsInterface;

  ASAN_CHECK_MEMORY_REGION (Part, sizeof (EXT4_PARTITION));

  This = (EFI_FILE_PROTOCOL *)Part->Root;

  //
  // Test Ext4Dxe driver
  //
  for (Index = 0; Index < OPEN_FILE_MODES_COUNT; Index++) {
    Status = Ext4Open (This, &NewHandle, FileName, mOpenFileModes[Index], 0);
    if (Status == EFI_SUCCESS) {
      Buffer     = NULL;
      BufferSize = 0;
      Status     = Ext4ReadFile (NewHandle, &BufferSize, Buffer);
      if (Status == EFI_BUFFER_TOO_SMALL) {
        Buffer = AllocateZeroPool (BufferSize);
        if (Buffer == NULL) {
          FreeAll (FileName, Part);
          return 0;
        }

        ASAN_CHECK_MEMORY_REGION (Buffer, BufferSize);

        Ext4ReadFile (NewHandle, &BufferSize, Buffer);

        Ext4WriteFile (NewHandle, &BufferSize, Buffer);

        FreePool (Buffer);
      }

      Len    = 0;
      Info   = NULL;
      Status = Ext4GetInfo (NewHandle, &gEfiFileInfoGuid, &Len, Info);
      if (Status == EFI_BUFFER_TOO_SMALL) {
        Info = AllocateZeroPool (Len);
        if (Info == NULL) {
          FreeAll (FileName, Part);
          return 0;
        }

        Ext4GetInfo (NewHandle, &gEfiFileInfoGuid, &Len, Info);
        FreePool (Info);
      }

      Len    = 0;
      Status = Ext4GetInfo (NewHandle, &gEfiFileSystemInfoGuid, &Len, Info);
      if (Status == EFI_BUFFER_TOO_SMALL) {
        Info = AllocateZeroPool (Len);
        if (Info == NULL) {
          FreeAll (FileName, Part);
          return 0;
        }

        Ext4GetInfo (NewHandle, &gEfiFileSystemInfoGuid, &Len, Info);
        FreePool (Info);
      }

      Len    = 0;
      Status = Ext4GetInfo (NewHandle, &gEfiFileSystemVolumeLabelInfoIdGuid, &Len, Info);
      if (Status == EFI_BUFFER_TOO_SMALL) {
        Info = AllocateZeroPool (Len);
        if (Info == NULL) {
          FreeAll (FileName, Part);
          return 0;
        }

        Ext4GetInfo (NewHandle, &gEfiFileSystemVolumeLabelInfoIdGuid, &Len, Info);
        FreePool (Info);
      }

      Ext4SetInfo (NewHandle, &gEfiFileSystemVolumeLabelInfoIdGuid, Len, Info);

      //
      // Test position functions
      //
      Ext4GetPosition (NewHandle, &Position);
      Ext4SetPosition (NewHandle, Position);

      //
      // Trying to reach the end of file and read/write it
      //
      Position = (UINT64)-1;
      Status   = Ext4SetPosition (NewHandle, Position);
      if (!EFI_ERROR (Status)) {
        Buffer     = NULL;
        BufferSize = 0;
        Status     = Ext4ReadFile (NewHandle, &BufferSize, Buffer);
        if (Status == EFI_BUFFER_TOO_SMALL) {
          Buffer = AllocateZeroPool (BufferSize);
          if (Buffer == NULL) {
            FreeAll (FileName, Part);
            return 0;
          }

          ASAN_CHECK_MEMORY_REGION (Buffer, BufferSize);

          Ext4ReadFile (NewHandle, &BufferSize, Buffer);

          Ext4WriteFile (NewHandle, &BufferSize, Buffer);

          FreePool (Buffer);
        }
      }

      //
      // Trying to reach out of bound of FileSize
      //
      File     = EXT4_FILE_FROM_THIS (NewHandle);
      FileSize = EXT4_INODE_SIZE (File->Inode);
      if (FileSize < (UINT64)-1 - 1) {
        Position = FileSize + 1;
        Status   = Ext4SetPosition (NewHandle, Position);
        if (!EFI_ERROR (Status)) {
          Buffer     = NULL;
          BufferSize = 0;
          Status     = Ext4ReadFile (NewHandle, &BufferSize, Buffer);

          ASSERT (Status == EFI_DEVICE_ERROR);
        }
      }

      Ext4Delete (NewHandle);
    }
  }

  FreeAll (FileName, Part);

  return 0;
}

INT32
LLVMFuzzerTestOneInput (
  CONST UINT8  *FuzzData,
  UINTN        FuzzSize
  )
{
  VOID  *NewData;

  if (FuzzSize == 0) {
    return 0;
  }

  //
  // Override InstallMultipleProtocolInterfaces with custom wrapper
  //
  gBS->InstallMultipleProtocolInterfaces = WrapInstallMultipleProtocolInterfaces;

  ConfigureMemoryAllocations (FuzzData, FuzzSize);

  NewData = AllocatePool (FuzzSize);
  if (NewData != NULL) {
    CopyMem (NewData, FuzzData, FuzzSize);
    TestExt4Dxe (NewData, FuzzSize);
    FreePool (NewData);
  }

  DEBUG ((
    DEBUG_POOL | DEBUG_PAGE,
    "UMEM: Allocated %u pools %u pages\n",
    (UINT32)mPoolAllocations,
    (UINT32)mPageAllocations
    ));

  return 0;
}

int
ENTRY_POINT (
  int   argc,
  char  **argv
  )
{
  uint32_t  f;
  uint8_t   *b;

  if ((b = UserReadFile ((argc > 1) ? argv[1] : "in.bin", &f)) == NULL) {
    DEBUG ((DEBUG_ERROR, "Read fail\n"));
    return -1;
  }

  LLVMFuzzerTestOneInput (b, f);
  FreePool (b);
  return 0;
}
