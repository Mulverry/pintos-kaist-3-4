#ifndef VM__H
#define VM__H
#include "vm/vm.h"
struct page;
enum vm_type;

struct anon_page {
    struct page *anon_p;
};

void vm__init (void);
bool _initializer (struct page *page, enum vm_type type, void *kva);

#endif
