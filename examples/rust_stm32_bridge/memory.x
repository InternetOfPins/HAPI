/* STM32F103C8 (Blue Pill) -- conservative documented sizes (64K/20K),
   not the 128K physically reported over SWD in focCompose's HANDOFF.md
   (marked/sold as C8T6 = 64K) -- stay within documented bounds. */
MEMORY
{
  FLASH : ORIGIN = 0x08000000, LENGTH = 64K
  RAM   : ORIGIN = 0x20000000, LENGTH = 20K
}
