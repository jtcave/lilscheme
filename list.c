#include "lilscheme.h"

// List processing

int ListLength(Handle list) {
    int len = 0;
    while (list != nil) {
	list = Cdr(list);
	len++;
    }
    return len;
}


// equivalent to `assq`
Handle AlistGet(Handle alist, Handle key) {
    while (alist != nil) {
	Handle cell = Car(alist);
	if (Car(cell) == key) {
	    return cell;
	}
	else alist = Cdr(alist);
    }
    return nil;
}

// returns the new alist
Handle AlistSet(Handle alist, Handle key, Handle value) {
    Handle cell = AlistGet(alist, key);
    if (cell != nil) {
	SetCdr(cell, value);
	return alist;
    }
    else {
	Retain(value);
	cell = CreateCons(key, value);
	Retain(cell);
	Unretain(value);
	Handle head = CreateCons(cell, alist);
	Unretain(cell);
	return head;
    }
}
