#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char *argv[]) {
    printf("Usage: %s -n -f <database file>\n", argv[0]);
    printf("\t -n - create new database file\n");
    printf("\t -f - (required) path to database file\n");
    printf("\t -a - add employee using comma seperated string 'Josh,41 Billy Joe Lane, 200'\n");
    printf("\t -l - list all employees in the database'\n");
    printf("\t -s - used in conjunction with -h to update hours\n");
    printf("\t -r - remove employee by name\n");
    return;
}

int main(int argc, char *argv[]) { 
    int c;
    int dbfd;
    bool newfile = false;
    char *filepath = NULL;
    char *addstring = NULL;
    char *searchstring = NULL;
    char *hoursstring = NULL;
    char *removestring = NULL;
    bool listemployees = false;
    struct dbheader_t *dbhdr = NULL;
    struct employee_t *employees = NULL;

    while ((c = getopt(argc, argv, "nf:a:ls:h:r:")) != -1) {
        switch(c) {
            case 'n':
                newfile = true;
                break;
            case 'f':
                filepath = optarg;
                break;
            case 'a':
                addstring = optarg;
                break;
            case 'l':
                listemployees = true;
                break;
            case 's':
                searchstring = optarg;
                break;
            case 'h':
                hoursstring = optarg;
                break;
            case 'r':
                removestring = optarg;
                break;
            case '?':
                printf("Unknown option -%c\n", c);
                break;
            default:
                return -1;
        }
    }

    if (filepath == NULL) {
        printf("Filepath is a required argument\n");
        print_usage(argv);
        return 0;
    }

    if (newfile) {
        dbfd = create_db_file(filepath);
        if (dbfd == STATUS_ERROR) {
            printf("Unable to create database file\n");
            return -1;
        }
        if (create_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
            printf("Failed to create database header\n");
            return -1;
        }
    } else {
        dbfd = open_db_file(filepath);
        if (dbfd == STATUS_ERROR) {
            printf("Unable to create database file\n");
            return -1;
        }
        if (validate_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
            printf("Failed to validate database header\n");
            return -1;
        }
    }

    if (read_employees(dbfd, dbhdr, &employees) != STATUS_SUCCESS) {
        printf("Failed to read employees\n");
        return 0;
    }

    if (addstring) {
        dbhdr->count++;
        employees = realloc(employees, dbhdr->count * (sizeof(struct employee_t)));
        add_employee(dbhdr, employees, addstring);
    }

    if (listemployees) {
        list_employees(dbhdr, employees);
    }

    if (searchstring && hoursstring) {
        update_employee_hours(employees, dbhdr, searchstring, hoursstring);
    }

    if (removestring) {
        remove_employee_by_name(employees, dbhdr, removestring);
    }

    output_file(dbfd, dbhdr, employees);

    close(dbfd);

    return 0;
	
}
