/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include "lib/kernel/bitmap.h"

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
struct bitmap *swap_table;

static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	swap_disk = disk_get(1, 1);
	size_t swap_size = disk_size(swap_disk) / (PGSIZE/DISK_SECTOR_SIZE); //  swap_disk / 페이지 당 디스크 섹터 수
	struct bitmap *swap_table = bitmap_create(swap_size);
	bitmap_set_all(swap_table, 0);
}

/* Initialize the file mapping */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
	size_t index = anon_page->idx;
	if (!bitmap_test(swap_table, index)) return false;
	for (int i = 0; i < (PGSIZE/DISK_SECTOR_SIZE); i++){
		disk_read(swap_disk, index*(PGSIZE/DISK_SECTOR_SIZE) + i, kva+i*DISK_SECTOR_SIZE);
	}
	bitmap_set(swap_table, index, false);
	return true;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;
	size_t index = bitmap_scan(swap_table, 0, 1, false);
	if (index == BITMAP_ERROR) return false;
	for (int i=0; i<(PGSIZE/DISK_SECTOR_SIZE); i++){
		disk_write(swap_disk, index*(PGSIZE/DISK_SECTOR_SIZE)+i, page->va + i*DISK_SECTOR_SIZE);
	}
	bitmap_set(swap_table, index, true); // 디스크 사용 중이라고 표시
	pml4_clear_page(thread_current()->pml4, page->va); // 페이지 테이블에서 해당 가상 주소에 대한 매핑을 제거
	anon_page->idx = index; //  익명 페이지가 스왑 슬롯 index에 스왑아웃됨을 표시
	return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}
