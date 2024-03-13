#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

#include "vm/quivm.h"
#include "dev/storage.h"

/* Constants */
#define MAX_FILENAME_LENGTH            512

/* Functions */

int storage_init(struct storage *stg)
{
    stg->name = 0;
    stg->data = 0;
    stg->len = 0;
    stg->op = 0;
    stg->disable_write = 0;
    return 0;
}

void storage_destroy(struct storage *stg)
{
    (void)(stg); /* UNUSED */
}

uint32_t storage_read_callback(const struct storage *stg,
                               const struct quivm *qvm, uint32_t address)
{
    uint32_t v;
    (void)(qvm); /* UNUSED */

    switch (address) {
    case IO_STORAGE_NAME:
        v = stg->name;
        break;
    case IO_STORAGE_DATA:
        v = stg->data;
        break;
    case IO_STORAGE_LEN:
        v = stg->len;
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
 * buffer `filename`, which is of size `size`.
 * Return zero on success.
 */
static
int get_filename(struct storage *stg, struct quivm *qvm,
                 char *filename, uint32_t size)
{
    uint32_t i, len, address;
    address = stg->name;
    for (i = 0; i < size - 1; i++) {
        /* Check if reading from memory */
        if (!(address < qvm->memsize)) break;

        filename[i] = quivm_read_byte(qvm, address++);
        if (filename[i] == '\0') break;
    }
    len = i;
    filename[len] = '\0'; /* make sure it is null terminated */

    /* now validate the filename
     * It must contain only alphanumeric characters or the dot '.'
     * character. And it must not start with a dot.
     */
    if (len == 0) return -1;
    if (!isalnum(filename[0])) return -2;
    for (i = 1; i < len; i++) {
        if (filename[i] != '.' && !isalnum(filename[i]))
            return -3;
    }

    return 0;
}

/* Performs the file operation */
static
void do_operation(struct storage *stg, struct quivm *qvm)
{
    FILE *fp;
    char filename[MAX_FILENAME_LENGTH];
    uint32_t i, address;
    int c;

    if (stg->op != STORAGE_OP_READ && stg->op != STORAGE_OP_WRITE) {
        stg->len = 0;
        return;
    }

    if (get_filename(stg, qvm, filename, sizeof(filename))) {
        /* Length 0 indicates an error */
        stg->len = 0;
        return;
    }

    switch (stg->op) {
    case STORAGE_OP_READ:
        fp = fopen(filename, "r");
        address = stg->data;
        for (i = 0; i < stg->len; i++) {
            c = fgetc(fp);
            if (c == EOF) break;

            /* Check if writing to memory */
            if (!(address < qvm->memsize)) break;
            quivm_write_byte(qvm, address++, (uint8_t) c);
        }
        stg->len = i;
        fclose(fp);
        break;
    case STORAGE_OP_WRITE:
        if (stg->disable_write) {
            /* Length 0 indicates an error */
            stg->len = 0;
            break;
        }

        fp = fopen(filename, "w");
        address = stg->data;
        for (i = 0; i < stg->len; i++) {
            /* Check if reading from memory */
            if (!(address < qvm->memsize)) break;

            c = quivm_read_byte(qvm, address++);
            c = fputc(c, fp);
            if (c == EOF) break;
        }
        stg->len = i;
        fclose(fp);
        break;
    }
}

void storage_write_callback(struct storage *stg,  struct quivm *qvm,
                            uint32_t address, uint32_t v)
{
    switch (address) {
    case IO_STORAGE_NAME:
        stg->name = v;
        break;
    case IO_STORAGE_DATA:
        stg->data = v;
        break;
    case IO_STORAGE_LEN:
        stg->len = v;
        break;
    case IO_STORAGE_OP:
        stg->op = v;
        do_operation(stg, qvm);
        break;
    }
}
