#ifndef VM__H
#define VM__H
#include "vm/vm.h"
struct page;
enum vm_type;

struct anon_page {
    struct page *page;
};

void vm__init (void);
bool _initializer (struct page *page, enum vm_type type, void *kva);

#endif
