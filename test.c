#include "j4on.h"

int main() {
    struct json json;
    j4on_load(&json, "foo.json");
    struct slist list, *pos;
    slist_init(&list);
}