/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "userprog/process.h"
#include "threads/mmu.h"
#include "threads/malloc.h"
#include "userprog/syscall.h"

static bool file_backed_swap_in(struct page *page, void *kva);
static bool file_backed_swap_out(struct page *page);
static void file_backed_destroy(struct page *page);
void do_munmap(void *addr);
void *do_mmap(void *addr, size_t length, int writable, struct file *file, off_t offset);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void vm_file_init(void)
{
}

/* 파일 백업 페이지 초기화 */
bool file_backed_initializer(struct page *page, enum vm_type type, void *kva)
{
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in(struct page *page, void *kva)
{
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out(struct page *page)
{
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy(struct page *page)
{
	struct file_page *file_page UNUSED = &page->file;
}
/*---------------------------Mapped Files----------------------------------------- */
/* Do the mmap */
// Sangju
// load_segment() -> ANON 설정
// do_mmap -> 파일 인자  넘김
void *
do_mmap(void *addr, size_t length, int writable,
		struct file *file, off_t offset)
{
	// 인자로 들어온 파일을 reopen 함수 사용해서 동일한 파일에 대해 다른 주소를 가지는 파일 구조체 생성
	// reopen()하는 이유는 mmap을 하는 동안 만약 외부에서 해당 파일을 close()하는 불상사가 어쩌고 저쩌고
	struct file *copyfile = file_reopen(file);
	// 초기 addr 저장용 후에 return 으로 반환
	void *save_addr = addr;
	int cnt;
	if (length % PGSIZE != 0)
	{
		cnt = (length / PGSIZE) + 1;
	}
	else
	{
		cnt = (length / PGSIZE);
	}
	// 파일길이랑 length를 비교 작으면 그대로 더 크면 length만큼
	size_t read_bytes;
	if (file_length(file) < length)
	{
		read_bytes = file_length(file);
	}
	else
	{
		read_bytes = length;
	}
	size_t zero_bytes = PGSIZE - read_bytes % PGSIZE;

	ASSERT((read_bytes + zero_bytes) % PGSIZE == 0);
	ASSERT(pg_ofs(addr) == 0);
	ASSERT(offset % PGSIZE == 0);

	while (read_bytes > 0 || zero_bytes > 0)
	{

		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		struct segment *seg = (struct segment *)malloc(sizeof(struct segment));
		seg->file = copyfile;
		seg->ofs = offset;
		seg->read_bytes = page_read_bytes;
		// Sangju process.h segment 구조체에 file_cnt 추가했음,
		// process.c load_segment에서도 file_cnt에 값 넘겨주는거 추가해줘야함 짜피 안써서 0으로 박음
		seg->file_cnt = cnt;
		if (!vm_alloc_page_with_initializer(VM_FILE, addr,
											writable, lazy_load_segment, seg))
			return NULL;

		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		addr += PGSIZE;
		offset += page_read_bytes;
	}
	return save_addr;
}
// v3 file_cnt값 넘겨주기  , 리스트로 담아두기도 가능할듯?
void do_munmap(void *addr)
{
	struct thread *t = thread_current();
	struct page *page = spt_find_page(&t->spt, addr);
	if (page == NULL)
	{
		return NULL;
	}
	struct segment *seg = (struct segment *)page->uninit.aux;
	int file_cnt = seg->file_cnt;
	for (int i = 0; i < file_cnt; i++)
	{
		struct page *p = spt_find_page(&t->spt, addr + (PGSIZE * i));
		if (pml4_is_dirty(t->pml4, addr))
		{
			file_seek(seg->file, seg->ofs);
			file_write(seg->file, addr, seg->read_bytes);
			pml4_set_dirty(t->pml4, addr, false);
		}
		pml4_clear_page(t->pml4, addr);
	}
}

/*---------------------------Mapped Files----------------------------------------- */

/* mmap_file의 vme_list에 연결된 모든 vm_entry들을 제거 */
/* Do the munmap */
// Sangju v1 21개
// void do_munmap(void *addr)
// { // 연결된 물리프레임과의 연결을 끊어줌
// 	struct thread *t = thread_current();
// 	struct supplemental_page_table *src = &t->spt;
// 	struct hash_iterator i;
// 	hash_first(&i, &t->spt.spt_hash);
// 	while (hash_next(&i))
// 	{

// 		struct page *p1 = hash_entry(hash_cur(&i), struct page, hash_elem);
// 		struct page *p2 = spt_find_page(src, p1->va);
// 		struct file *file = ((struct segment *)p2)->file;
// 		off_t offset = ((struct segment *)p2)->ofs;
// 		size_t read_bytes = ((struct segment *)p2)->read_bytes;

// 		if (pml4_is_dirty(t->pml4, p2->va))
// 		{
// 			file_write_at(file, p2->frame->kva, read_bytes, offset);
// 			pml4_set_dirty(t->pml4, p2->va, false);
// 		}
// 		hash_delete(&t->spt.spt_hash, &p2->hash_elem);
// 	}
// 	// pml4_set_dirty()
// }

/*---------------------------Mapped Files----------------------------------------- */
// Sanju v2
// void do_munmap(void *addr)
// { // 연결된 물리프레임과의 연결을 끊어줌
// 	while (true)
// 	{
// 		struct thread *t = thread_current();
// 		struct page *p = spt_find_page(&t->spt, addr);
// 		if (p == NULL)
// 		{
// 			return NULL;
// 		}

// 		struct segment *seg = (struct segment *)p->uninit.aux;
// 		if (pml4_is_dirty(t->pml4, p->va))
// 		{
// 			file_write_at(seg->file, addr, seg->read_bytes, seg->ofs);
// 			pml4_set_dirty(t->pml4, p->va, false);
// 		}
// 		pml4_clear_page(t->pml4, p->va);
// 		addr += PGSIZE;
// 		// p = spt_find_page(&t->spt, p->va);
// 	}
// 	// pml4_set_dirty()
// }
