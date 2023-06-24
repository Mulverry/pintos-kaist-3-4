/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "userprog/process.h"
#include "userprog/syscall.h"
#include "threads/mmu.h"
#include "threads/malloc.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);
void *do_mmap (void *addr, size_t length, int writable, struct file *file, off_t offset);
void do_munmap(void *addr);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Do the mmap */
void *do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {

	void *init_addr = addr; // 초기 addr 저장용 후에 return 으로 반환
	size_t read_bytes = length;

	while (read_bytes > 0) {
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;

		struct segment *seg = (struct segment*)malloc(sizeof(struct segment));
		seg->file = file;
		seg->page_read_bytes = page_read_bytes;
		seg->ofs = offset;

		if (!vm_alloc_page_with_initializer(VM_FILE, addr, writable, lazy_load_segment, seg)){
			free(seg);
			return;
		}

	// 	/* Advance. */
		read_bytes -= page_read_bytes;
		addr += PGSIZE;
		offset += page_read_bytes;
	}
	return init_addr;
}

/* Do the munmap */
void do_munmap (void *addr) {
	struct thread *t = thread_current();
	struct page *page = spt_find_page(&t->spt, addr);
	if (page == NULL) return NULL;

	struct segment *seg = (struct segment *)page->uninit.aux;
	if (!seg->file) return;

	while (page != NULL){
		if (pml4_is_dirty(t->pml4, addr)){
			lock_acquire(&filesys_lock);
			file_seek(seg->file, seg->ofs);
			file_write(seg->file, addr, seg->page_read_bytes);
			lock_release(&filesys_lock);
			// pml4_set_dirty(t->pml4, addr, false);
		}
		pml4_clear_page(t->pml4, addr);
		addr += PGSIZE;
		page = spt_find_page(&t->spt, addr);
	}
}
