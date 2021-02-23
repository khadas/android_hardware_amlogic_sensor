/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <libboot_control/libboot_control.h>

#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include <string>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/unique_fd.h>
#include <bootloader_message/bootloader_message.h>

#include "private/boot_control_definition.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ubootenv/Ubootenv.h"

namespace android {
namespace bootable {

using ::android::hardware::boot::V1_1::MergeStatus;

// The number of boot attempts that should be made from a new slot before
// rolling back to the previous slot.
constexpr unsigned int kDefaultBootAttempts = 7;

#define EMMC_USER_PARTITION        "bootloader"
#define EMMC_BLK0BOOT0_PARTITION   "mmcblk0boot0"
#define EMMC_BLK0BOOT1_PARTITION   "mmcblk0boot1"
#define EMMC_BLK1BOOT0_PARTITION   "mmcblk1boot0"
#define EMMC_BLK1BOOT1_PARTITION   "mmcblk1boot1"

#define BOOTLOADER_MAX_SIZE    (4*1024*1024)
/*First 512 bytes in bootloader is signed data*/
#define BOOTLOADER_OFFSET      512

static const char *sEmmcPartionName_a[] = {
    EMMC_BLK0BOOT0_PARTITION,
    EMMC_BLK1BOOT0_PARTITION,
};
static const char *sEmmcPartionName_b[] = {
    EMMC_BLK0BOOT1_PARTITION,
    EMMC_BLK1BOOT1_PARTITION,
};

static_assert(kDefaultBootAttempts < 8, "tries_remaining field only has 3 bits");

constexpr unsigned int kMaxNumSlots =
    sizeof(bootloader_control::slot_info) / sizeof(bootloader_control::slot_info[0]);
constexpr const char* kSlotSuffixes[kMaxNumSlots] = { "_a", "_b", "_c", "_d" };
constexpr off_t kBootloaderControlOffset = offsetof(bootloader_message_ab, slot_suffix);

static uint32_t CRC32(const uint8_t* buf, size_t size) {
  static uint32_t crc_table[256];

  // Compute the CRC-32 table only once.
  if (!crc_table[1]) {
    for (uint32_t i = 0; i < 256; ++i) {
      uint32_t crc = i;
      for (uint32_t j = 0; j < 8; ++j) {
        uint32_t mask = -(crc & 1);
        crc = (crc >> 1) ^ (0xEDB88320 & mask);
      }
      crc_table[i] = crc;
    }
  }

  uint32_t ret = -1;
  for (size_t i = 0; i < size; ++i) {
    ret = (ret >> 8) ^ crc_table[(ret ^ buf[i]) & 0xFF];
  }

  return ~ret;
}

// Return the little-endian representation of the CRC-32 of the first fields
// in |boot_ctrl| up to the crc32_le field.
uint32_t BootloaderControlLECRC(const bootloader_control* boot_ctrl) {
  return htole32(
      CRC32(reinterpret_cast<const uint8_t*>(boot_ctrl), offsetof(bootloader_control, crc32_le)));
}

bool LoadBootloaderControl(const std::string& misc_device, bootloader_control* buffer) {
  android::base::unique_fd fd(open(misc_device.c_str(), O_RDONLY));
  if (fd.get() == -1) {
    PLOG(ERROR) << "failed to open " << misc_device;
    return false;
  }
  if (lseek(fd, kBootloaderControlOffset, SEEK_SET) != kBootloaderControlOffset) {
    PLOG(ERROR) << "failed to lseek " << misc_device;
    return false;
  }
  if (!android::base::ReadFully(fd.get(), buffer, sizeof(bootloader_control))) {
    PLOG(ERROR) << "failed to read " << misc_device;
    return false;
  }
  return true;
}

bool UpdateAndSaveBootloaderControl(const std::string& misc_device, bootloader_control* buffer) {
  buffer->crc32_le = BootloaderControlLECRC(buffer);
  android::base::unique_fd fd(open(misc_device.c_str(), O_WRONLY | O_SYNC));
  if (fd.get() == -1) {
    PLOG(ERROR) << "failed to open " << misc_device;
    return false;
  }
  if (lseek(fd.get(), kBootloaderControlOffset, SEEK_SET) != kBootloaderControlOffset) {
    PLOG(ERROR) << "failed to lseek " << misc_device;
    return false;
  }
  if (!android::base::WriteFully(fd.get(), buffer, sizeof(bootloader_control))) {
    PLOG(ERROR) << "failed to write " << misc_device;
    return false;
  }
  return true;
}

bool write_bootloader_img(unsigned int slot)
{
    int iRet = 0;
    char emmcPartitionPath[128];
    const char **sEmmcPartionName;
    int fd = -1;
    int fd2 = -1;
    bool ret = false;
    char* data = NULL;

    memset(emmcPartitionPath, 0, sizeof(emmcPartitionPath));
    if (slot == 0) {
        sEmmcPartionName = sEmmcPartionName_a;
    } else {
        sEmmcPartionName = sEmmcPartionName_b;
    }

    data = (char *)malloc(BOOTLOADER_MAX_SIZE);
    if (data == NULL) {
        LOG(ERROR) << "malloc " << BOOTLOADER_MAX_SIZE << " error";
        goto done;
    }
    memset(data, 0, BOOTLOADER_MAX_SIZE);

    for (int i = 0; i < 2; i ++) {
        sprintf(emmcPartitionPath, "/dev/block/%s", sEmmcPartionName[i]);
        LOG(INFO) << "emmcPartitionPath: " << emmcPartitionPath;
        /* get the bootloader path we write in update_engine
         * /dev/block/platform/soc/fe08c000.mmc/by-name/bootloader --> /dev/block/bootloader
         * /dev/block/platform/soc/fe08c000.mmc/by-name/bootloader_a --> /dev/block/mmcblk0boot0
         * /dev/block/platform/soc/fe08c000.mmc/by-name/bootloader_b --> /dev/block/mmcblk0boot1
         * for different board, mmcblk0/fe08c000 maybe different
         * update_engine will update bootloader_a or bootloader_b according to current slot
        */
        if (!access(emmcPartitionPath, F_OK)) {
            LOG(INFO) << "find " << emmcPartitionPath;
            break;
        }
    }

    fd = open(emmcPartitionPath, O_RDWR);
    if (fd < 0) {
        LOG(ERROR) << "failed to open " << emmcPartitionPath;
        goto done;
    }
    lseek(fd, 0, SEEK_SET);
    iRet = read(fd, data, BOOTLOADER_MAX_SIZE);
    if (iRet == BOOTLOADER_MAX_SIZE) {
        LOG(INFO) << "read bootloader img successful";
    } else {
        LOG(ERROR) << "read bootloader img failed";
        goto done;
    }
    /* First 512 bytes in bootloader is signed data
     * we need write bootloader.img to 512 bytes offset
     * but update_engine just write bootloader.img to /dev/block
     * so we read the data and rewrite it here
    */
    iRet = lseek(fd, BOOTLOADER_OFFSET, SEEK_SET);
    if (iRet == -1) {
        printf("failed to lseek %d\n", BOOTLOADER_OFFSET);
        goto done;
    }
    iRet = write(fd, data, BOOTLOADER_MAX_SIZE - BOOTLOADER_OFFSET);
    if (iRet == (BOOTLOADER_MAX_SIZE - BOOTLOADER_OFFSET)) {
        LOG(INFO) << "write bootloader img successful";
    } else {
        LOG(ERROR) << "write bootloader img failed";
        goto done;
    }

    /* We use robust to rollback bootloader.img in uboot
     * bootloader  ---> 0
     * boot0       ---> 1
     * boot1       ---> 2
     * It alway bootup from 0 first.
     * and will switch bootloader by the order: 0->1->2->0 ...
     * So we write bootloader.img to bootloader here, and set env
     * reboot_status  ---- reboot_next
     * expect_index   ---- 0
     * update_env     ---- 1
     * after reboot, if the bootloader index is just expect_index, update env too
     * if the bootloader index isn't expect_index, update error, back to last slot
    */
    fd2 = open("/dev/block/bootloader", O_RDWR);
    if (fd2 < 0) {
        LOG(ERROR) << "failed to open /dev/block/bootloader ";
        goto done;
    }
    iRet = lseek(fd2, BOOTLOADER_OFFSET, SEEK_SET);
    if (iRet == -1) {
        printf("failed to lseek %d\n", BOOTLOADER_OFFSET);
        goto done;
    }
    iRet = write(fd2, data, BOOTLOADER_MAX_SIZE - BOOTLOADER_OFFSET);
    if (iRet == (BOOTLOADER_MAX_SIZE - BOOTLOADER_OFFSET)) {
        LOG(INFO) << "Write Uboot Image successful";
    } else {
        LOG(ERROR) << "Write Uboot Image failed";
        goto done;
    }

    ret = true;

done:
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
    if (fd2 >= 0) {
        close(fd2);
        fd2 = -1;
    }
    if (data != NULL) {
        free(data);
        data = NULL;
    }
    return ret;
}

int set_bootloader_env(const char* name, const char* value)
{
    Ubootenv *ubootenv = new Ubootenv();
    char ubootenv_name[128] = {0};
    const char *ubootenv_var = "ubootenv.var.";
    sprintf(ubootenv_name, "%s%s", ubootenv_var, name);

    if (ubootenv->updateValue(ubootenv_name, value)) {
        PLOG(ERROR) << "could not set boot env";
        return -1;
    }
    return 0;
}

char* get_bootloader_env(const char * name)
{
    Ubootenv *ubootenv = new Ubootenv();
    char ubootenv_name[128] = {0};
    const char *ubootenv_var = "ubootenv.var.";
    sprintf(ubootenv_name, "%s%s", ubootenv_var, name);
    return (char *)ubootenv->getValue(ubootenv_name);
}

void InitDefaultBootloaderControl(BootControl* control, bootloader_control* boot_ctrl) {
  memset(boot_ctrl, 0, sizeof(*boot_ctrl));

  unsigned int current_slot = control->GetCurrentSlot();
  if (current_slot < kMaxNumSlots) {
    strlcpy(boot_ctrl->slot_suffix, kSlotSuffixes[current_slot], sizeof(boot_ctrl->slot_suffix));
  }
  boot_ctrl->magic = BOOT_CTRL_MAGIC;
  boot_ctrl->version = BOOT_CTRL_VERSION;

  // Figure out the number of slots by checking if the partitions exist,
  // otherwise assume the maximum supported by the header.
  boot_ctrl->nb_slot = kMaxNumSlots;
  std::string base_path = control->misc_device();
  size_t last_path_sep = base_path.rfind('/');
  if (last_path_sep != std::string::npos) {
    // We test the existence of the "boot" partition on each possible slot,
    // which is a partition required by Android Bootloader Requirements.
    base_path = base_path.substr(0, last_path_sep + 1) + "boot";
    int last_existing_slot = -1;
    int first_missing_slot = -1;
    for (unsigned int slot = 0; slot < kMaxNumSlots; ++slot) {
      std::string partition_path = base_path + kSlotSuffixes[slot];
      struct stat part_stat;
      int err = stat(partition_path.c_str(), &part_stat);
      if (!err) {
        last_existing_slot = slot;
        LOG(INFO) << "Found slot: " << kSlotSuffixes[slot];
      } else if (err < 0 && errno == ENOENT && first_missing_slot == -1) {
        first_missing_slot = slot;
      }
    }
    // We only declare that we found the actual number of slots if we found all
    // the boot partitions up to the number of slots, and no boot partition
    // after that. Not finding any of the boot partitions implies a problem so
    // we just leave the number of slots in the maximum value.
    if ((last_existing_slot != -1 && last_existing_slot + 1 == first_missing_slot) ||
        (first_missing_slot == -1 && last_existing_slot + 1 == kMaxNumSlots)) {
      boot_ctrl->nb_slot = last_existing_slot + 1;
      LOG(INFO) << "Found a system with " << last_existing_slot + 1 << " slots.";
    }
  }

  for (unsigned int slot = 0; slot < kMaxNumSlots; ++slot) {
    slot_metadata entry = {};

    if (slot < boot_ctrl->nb_slot) {
      entry.priority = 7;
      entry.tries_remaining = kDefaultBootAttempts;
      entry.successful_boot = 0;
    } else {
      entry.priority = 0;  // Unbootable
    }

    // When the boot_control stored on disk is invalid, we assume that the
    // current slot is successful. The bootloader should repair this situation
    // before booting and write a valid boot_control slot, so if we reach this
    // stage it means that the misc partition was corrupted since boot.
    if (current_slot == slot) {
      entry.successful_boot = 1;
    }

    boot_ctrl->slot_info[slot] = entry;
  }
  boot_ctrl->recovery_tries_remaining = 0;

  boot_ctrl->crc32_le = BootloaderControlLECRC(boot_ctrl);
}

// Return the index of the slot suffix passed or -1 if not a valid slot suffix.
int SlotSuffixToIndex(const char* suffix) {
  for (unsigned int slot = 0; slot < kMaxNumSlots; ++slot) {
    if (!strcmp(kSlotSuffixes[slot], suffix)) return slot;
  }
  return -1;
}

// Initialize the boot_control_private struct with the information from
// the bootloader_message buffer stored in |boot_ctrl|. Returns whether the
// initialization succeeded.
bool BootControl::Init() {
  if (initialized_) return true;

  // Initialize the current_slot from the read-only property. If the property
  // was not set (from either the command line or the device tree), we can later
  // initialize it from the bootloader_control struct.
  std::string suffix_prop = android::base::GetProperty("ro.boot.slot_suffix", "");
  if (suffix_prop.empty()) {
    LOG(ERROR) << "Slot suffix property is not set";
    return false;
  }
  current_slot_ = SlotSuffixToIndex(suffix_prop.c_str());

  std::string err;
  std::string device = get_bootloader_message_blk_device(&err);
  if (device.empty()) {
    LOG(ERROR) << "Could not find bootloader message block device: " << err;
    return false;
  }

  bootloader_control boot_ctrl;
  if (!LoadBootloaderControl(device.c_str(), &boot_ctrl)) {
    LOG(ERROR) << "Failed to load bootloader control block";
    return false;
  }

  // Note that since there isn't a module unload function this memory is leaked.
  // We use `device` below sometimes, so it's not moved out of here.
  misc_device_ = device;
  initialized_ = true;

  // Validate the loaded data, otherwise we will destroy it and re-initialize it
  // with the current information.
  uint32_t computed_crc32 = BootloaderControlLECRC(&boot_ctrl);
  if (boot_ctrl.crc32_le != computed_crc32) {
    LOG(WARNING) << "Invalid boot control found, expected CRC-32 0x" << std::hex << computed_crc32
                 << " but found 0x" << std::hex << boot_ctrl.crc32_le << ". Re-initializing.";
    InitDefaultBootloaderControl(this, &boot_ctrl);
    UpdateAndSaveBootloaderControl(device.c_str(), &boot_ctrl);
  }

  if (!InitMiscVirtualAbMessageIfNeeded()) {
    return false;
  }

  num_slots_ = boot_ctrl.nb_slot;
  return true;
}

unsigned int BootControl::GetNumberSlots() {
  return num_slots_;
}

unsigned int BootControl::GetCurrentSlot() {
  return current_slot_;
}

bool BootControl::MarkBootSuccessful() {
  bootloader_control bootctrl;
  if (!LoadBootloaderControl(misc_device_, &bootctrl)) return false;

  bootctrl.slot_info[current_slot_].successful_boot = 1;
  // tries_remaining == 0 means that the slot is not bootable anymore, make
  // sure we mark the current slot as bootable if it succeeds in the last
  // attempt.
  bootctrl.slot_info[current_slot_].tries_remaining = 1;
  return UpdateAndSaveBootloaderControl(misc_device_, &bootctrl);
}

bool BootControl::SetBootloaderIndex() {
  set_bootloader_env("reboot_status", "reboot_next");
  set_bootloader_env("expect_index", "0");
  set_bootloader_env("update_env", "1");
  return true;
}

bool BootControl::SetActiveBootSlot(unsigned int slot) {
  if (slot >= kMaxNumSlots || slot >= num_slots_) {
    // Invalid slot number.
    return false;
  }

  bootloader_control bootctrl;
  bool ret;
  if (!LoadBootloaderControl(misc_device_, &bootctrl)) return false;

  // Set every other slot with a lower priority than the new "active" slot.
  const unsigned int kActivePriority = 15;
  const unsigned int kActiveTries = 6;
  for (unsigned int i = 0; i < num_slots_; ++i) {
    if (i != slot) {
      if (bootctrl.slot_info[i].priority >= kActivePriority)
        bootctrl.slot_info[i].priority = kActivePriority - 1;
    }
  }

  // Note that setting a slot as active doesn't change the successful bit.
  // The successful bit will only be changed by setSlotAsUnbootable().
  bootctrl.slot_info[slot].priority = kActivePriority;
  bootctrl.slot_info[slot].tries_remaining = kActiveTries;

  // Setting the current slot as active is a way to revert the operation that
  // set *another* slot as active at the end of an updater. This is commonly
  // used to cancel the pending update. We should only reset the verity_corrpted
  // bit when attempting a new slot, otherwise the verity bit on the current
  // slot would be flip.
  if (slot != current_slot_) bootctrl.slot_info[slot].verity_corrupted = 0;

  ret = UpdateAndSaveBootloaderControl(misc_device_, &bootctrl);

  if (ret) {
    ret = write_bootloader_img(slot);
  }

  if (ret) {
    ret = SetBootloaderIndex();
  }

  return ret;
}

bool BootControl::SetSlotAsUnbootable(unsigned int slot) {
  if (slot >= kMaxNumSlots || slot >= num_slots_) {
    // Invalid slot number.
    return false;
  }

  bootloader_control bootctrl;
  if (!LoadBootloaderControl(misc_device_, &bootctrl)) return false;
  // The only way to mark a slot as unbootable, regardless of the priority is to
  // set the tries_remaining to 0.
  bootctrl.slot_info[slot].successful_boot = 0;
  bootctrl.slot_info[slot].tries_remaining = 0;
  return UpdateAndSaveBootloaderControl(misc_device_, &bootctrl);
}

bool BootControl::IsSlotBootable(unsigned int slot) {
  if (slot >= kMaxNumSlots || slot >= num_slots_) {
    // Invalid slot number.
    return false;
  }

  bootloader_control bootctrl;
  if (!LoadBootloaderControl(misc_device_, &bootctrl)) return false;

  return bootctrl.slot_info[slot].tries_remaining != 0;
}

bool BootControl::IsSlotMarkedSuccessful(unsigned int slot) {
  if (slot >= kMaxNumSlots || slot >= num_slots_) {
    // Invalid slot number.
    return false;
  }

  bootloader_control bootctrl;
  if (!LoadBootloaderControl(misc_device_, &bootctrl)) return false;

  return bootctrl.slot_info[slot].successful_boot && bootctrl.slot_info[slot].tries_remaining;
}

bool BootControl::IsValidSlot(unsigned int slot) {
  return slot < kMaxNumSlots && slot < num_slots_;
}

bool BootControl::SetSnapshotMergeStatus(MergeStatus status) {
  return SetMiscVirtualAbMergeStatus(current_slot_, status);
}

MergeStatus BootControl::GetSnapshotMergeStatus() {
  MergeStatus status;
  if (!GetMiscVirtualAbMergeStatus(current_slot_, &status)) {
    return MergeStatus::UNKNOWN;
  }
  return status;
}

const char* BootControl::GetSuffix(unsigned int slot) {
  if (slot >= kMaxNumSlots || slot >= num_slots_) {
    return nullptr;
  }
  return kSlotSuffixes[slot];
}

bool InitMiscVirtualAbMessageIfNeeded() {
  std::string err;
  misc_virtual_ab_message message;
  if (!ReadMiscVirtualAbMessage(&message, &err)) {
    LOG(ERROR) << "Could not read merge status: " << err;
    return false;
  }

  if (message.version == MISC_VIRTUAL_AB_MESSAGE_VERSION &&
      message.magic == MISC_VIRTUAL_AB_MAGIC_HEADER) {
    // Already initialized.
    return true;
  }

  message = {};
  message.version = MISC_VIRTUAL_AB_MESSAGE_VERSION;
  message.magic = MISC_VIRTUAL_AB_MAGIC_HEADER;
  if (!WriteMiscVirtualAbMessage(message, &err)) {
    LOG(ERROR) << "Could not write merge status: " << err;
    return false;
  }
  return true;
}

bool SetMiscVirtualAbMergeStatus(unsigned int current_slot,
                                 android::hardware::boot::V1_1::MergeStatus status) {
  std::string err;
  misc_virtual_ab_message message;

  if (!ReadMiscVirtualAbMessage(&message, &err)) {
    LOG(ERROR) << "Could not read merge status: " << err;
    return false;
  }

  message.merge_status = static_cast<uint8_t>(status);
  message.source_slot = current_slot;
  if (!WriteMiscVirtualAbMessage(message, &err)) {
    LOG(ERROR) << "Could not write merge status: " << err;
    return false;
  }
  return true;
}

bool GetMiscVirtualAbMergeStatus(unsigned int current_slot,
                                 android::hardware::boot::V1_1::MergeStatus* status) {
  std::string err;
  misc_virtual_ab_message message;

  if (!ReadMiscVirtualAbMessage(&message, &err)) {
    LOG(ERROR) << "Could not read merge status: " << err;
    return false;
  }

  // If the slot reverted after having created a snapshot, then the snapshot will
  // be thrown away at boot. Thus we don't count this as being in a snapshotted
  // state.
  *status = static_cast<MergeStatus>(message.merge_status);
  if (*status == MergeStatus::SNAPSHOTTED && current_slot == message.source_slot) {
    *status = MergeStatus::NONE;
  }
  return true;
}

}  // namespace bootable
}  // namespace android
