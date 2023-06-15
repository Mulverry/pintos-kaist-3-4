/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "kernel/hash.h"
#include "threads/vaddr.h"

unsigned get_hash_bytes(const struct hash_elem *spt_elem, void *aux UNUSED);
unsigned less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux);


/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* 이니셜라이저를 사용하여 보류 중인 페이지 개체를 만듭니다.
 * 페이지를 만드려는 경우, 직접 생성하지 말고 이 기능을 통해 생성하거나
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current()->spt_hash;
	struct page *page;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* 업게이지가 이미 사용 중인지 확인하십시오. */
		/* TODO: 페이지를 생성하고 VM 유형에 따라 이니셜라이저를 가져옵니다,
	 	/* TODO: 그런 다음 unit_new를 호출하여 "uninit" 페이지 구조를 만듭니다.
	 	/* TODO: unit_new를 호출한 후 필드를 수정해야 합니다. */
		// vm type(uninit, anon, file, page_cache)에 따라 initializer를 다르게 가져와야 한다는 뜻일까?

		page = (struct page *)malloc(sizeof(struct page));
		switch (VM_TYPE(type)){
			case VM_ANON:
				uninit_new(page, page->va, init, type, aux, anon_initializer(page, type, aux));
			case VM_FILE:
				uninit_new(page, page->va, init, type, aux, file_backed_initializer(page, type, aux));
		}
		page->writable = writable;
		return spt_insert_page(spt, page);
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	/* TODO: Fill this function. */
	struct page *page = (struct page*)malloc(sizeof(struct page)); //페이지 할당
	struct hash_elem *spt_elem;

	page->va = pg_round_down(va); //페이지 번호 얻기. 해당 페이지의 시작 부분.
	if (hash_find(&spt->spt, spt_elem == NULL)){
		free(page);
		return NULL;
	} else {
		spt_elem = hash_find(&spt->spt, &page->hash_elem); //hash_entry 리턴
		free(page);
		return hash_entry(spt_elem, struct page, hash_elem);
	}
	return page;
}

/* Insert PAGE into spt with validation. */
bool spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = true;
	/* TODO: Fill this function. */
	if (hash_insert(spt, &page->hash_elem)){
		return succ;
	}else {
		return false;
	} 
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	// vm_dealloc_page (page);
	if (hash_delete(spt, &page->hash_elem)){
		return true;
	} else {
		return false;
	}
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = palloc_get_page(PAL_ZERO | PAL_USER);
	/* TODO: Fill this function. */
	//YeonJu:
	frame->kva = NULL;

	ASSERT (frame != NULL); // 물리메모리 X
	ASSERT (frame->page == NULL); // 페이지 테이블 X
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	return vm_do_claim_page (page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy(page);
	free(page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function */

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */

	return swap_in(page, frame->kva);
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(spt, get_hash_bytes, less_func, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}

unsigned get_hash_bytes(const struct hash_elem *spt_elem, void *aux UNUSED){
	const struct page *p= hash_entry(spt_elem, struct page, hash_elem);
	return hash_bytes(&p->va, sizeof(p->va));
}

unsigned less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux){
	const struct page *p_a = hash_entry(a, struct page, hash_elem);
	const struct page *p_b = hash_entry(b, struct page, hash_elem);
	return p_a->va < p_b->va;
}