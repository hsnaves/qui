#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>

#include "vm/quivm.h"
#include "dev/storage.h"

/* Functions */

int storage_init(struct storage *stg)
{
    stg->name = 0;
    stg->namelen = 0;
    stg->data = 0;
    stg->len = 0;
    stg->offset = 0;
    stg->op = 0;
    stg->disable_write = 0;
    return 0;
}

void storage_destroy(struct storage *stg)
{
    (void)(stg); /* UNUSED */
}

void storage_configure(struct storage *stg, int disable_write)
{
    stg->disable_write = disable_write;
}

uint32_t storage_read_callback(struct storage *stg,
                               struct quivm *qvm, uint32_t address)
{
    uint32_t v;
    (void)(qvm); /* UNUSED */

    switch (address) {
    case IO_STORAGE_NAME:
        v = stg->name;
        break;
    case IO_STORAGE_NAMELEN:
        v = stg->namelen;
        break;
    case IO_STORAGE_DATA:
        v = stg->data;
        break;
    case IO_STORAGE_LEN:
        v = stg->len;
        break;
    case IO_STORAGE_OFFSET:
        v = stg->offset;
        break;
    case IO_STORAGE_OP:
        v = stg->op;
        break;
    default:
        v = -1;
        break;
    }

    return v;
}

/* Obtains the filename from the virtual machine.
 * This function also validates the filename.
 * The QUI vm is given by `qvm`. The output filename is written to the
 * buffer `filename`.
 * Return zero on success.
 */
static
int get_filename(struct storage *stg, struct quivm *qvm, char *filename)
{
    uint32_t i;
    char c;

    memcpy(filename, &qvm->mem[stg->name], stg->namelen);
    filename[stg->namelen] = '\0'; /* make sure it is null terminated */

    /* now validate the filename
     * It must contain only printable characters. It must not start with
     * a forward slash, and it must not contain a dot immediately
     * followed by another dot.
     */
    if (stg->namelen == 0) return 1;

    for (i = 0; i < stg->namelen; i++) {
        c = filename[i];
        if (!isprint(c)) return 1;

        if (i == 0) {
            if (c == '/') return 1;
        } else {
            if (c == '.') {
                if (filename[i - 1] == '.')
                    return 1;
            }
        }
    }

    return 0;
}

/* Performs the file operation */
static
void do_operation(struct storage *stg, struct quivm *qvm)
{
    FILE *fp;
    char filename[512];

    if (((stg->op != STORAGE_OP_READ) && (stg->op != STORAGE_OP_WRITE))) {
        /* invalid operation */
        stg->len = -1;
        return;
    }

    /* ensure data and name are completely inside the memory
     * and that the name is not too large
     */
    if (check_buffer(stg->data, stg->len, MEMORY_SIZE)
        || check_buffer(stg->name, stg->namelen, MEMORY_SIZE)
        || (stg->namelen >= sizeof(filename))) {
        stg->len = -1;
        return;
    }

    if (get_filename(stg, qvm, filename)) {
        /* Length -1 indicates an error */
        stg->len = -1;
        return;
    }

    switch (stg->op) {
    case STORAGE_OP_READ:
        fp = fopen(filename, "r");
        if (!fp) {
            stg->len = -1;
            break;
        }

        if (stg->offset != 0) {
            if (fseek(fp, (long) stg->offset, SEEK_SET)) {
                stg->len = -1;
                fclose(fp);
                break;
            }
        }
        stg->len = (uint32_t)
            fread((void *) &qvm->mem[stg->data], 1, stg->len, fp);
        fclose(fp);
        break;

    case STORAGE_OP_WRITE:
        if (stg->disable_write) {
            /* Length -1 indicates an error */
            stg->len = -1;
            break;
        }

        if (stg->offset != 0) {
            /* Open in append mode if offset is nonzero */
            fp = fopen(filename, "a");
        } else {
            fp = fopen(filename, "w");
        }
        if (!fp) {
            stg->len = -1;
            break;
        }

        stg->len = (uint32_t)
            fwrite((void *) &qvm->mem[stg->data], 1, stg->len, fp);
        fclose(fp);
        break;
    }
}

void storage_write_callback(struct storage *stg, struct quivm *qvm,
                            uint32_t address, uint32_t v)
{
    switch (address) {
    case IO_STORAGE_NAME:
        stg->name = v;
        break;
    case IO_STORAGE_NAMELEN:
        stg->namelen = v;
        break;
    case IO_STORAGE_DATA:
        stg->data = v;
        break;
    case IO_STORAGE_LEN:
        stg->len = v;
        break;
    case IO_STORAGE_OFFSET:
        stg->offset = v;
        break;
    case IO_STORAGE_OP:
        stg->op = v;
        do_operation(stg, qvm);
        break;
    }
}
