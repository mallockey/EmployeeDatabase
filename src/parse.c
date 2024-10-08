#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "parse.h"

#define HEADER_VERSION 1

void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {
    int i = 0;
    for (; i < dbhdr->count; i++) {
        printf("Employee %d:\n", i);
        printf("\tName: %s\n", employees[i].name);
        printf("\tAddress: %s\n", employees[i].address);
        printf("\tHours: %d\n", employees[i].hours);
	}
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *addstring) {
    char *name = strtok(addstring, ",");
    char *addr = strtok(NULL, ",");
    char *hours = strtok(NULL, ",");

    int last_element = dbhdr->count - 1;

    strncpy(employees[last_element].name, name, sizeof(employees[last_element].name));
    strncpy(employees[last_element].address, addr, sizeof(employees[last_element].address));
    employees[last_element].hours = atoi(hours);

    return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {
    if (fd < 0) {
        printf("Got a bad FD from the user\n");
        return STATUS_ERROR;
    }

    int count = dbhdr->count;
    struct employee_t *employees = calloc(count, sizeof(struct employee_t));

    if (employees == -1){
        printf("Failed\n");
        return STATUS_ERROR;
    }

    read(fd, employees, count * sizeof(struct employee_t));

    int i = 0;

    for (; i < count; i++) {
        employees[i].hours = ntohl(employees[i].hours);
    }

    *employeesOut = employees;

    return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees) {
	if (fd < 0) {
		printf("Got a bad FD from the user\n");
		return STATUS_ERROR;
	}

	int realcount = dbhdr->count;

    off_t new_filesize = sizeof(struct dbheader_t) + (dbhdr->count * sizeof(struct employee_t));

    if (ftruncate(fd, new_filesize) == -1) {
        perror("ftruncate");
        return STATUS_ERROR;
    }

	lseek(fd, 0, SEEK_SET);

	dbhdr->magic = htonl(dbhdr->magic);
	dbhdr->filesize = htonl(sizeof(struct dbheader_t) + (sizeof(struct employee_t) * realcount));
	dbhdr->count = htons(dbhdr->count);
	dbhdr->version = htons(dbhdr->version);

	write(fd, dbhdr, sizeof(struct dbheader_t));

	for (int i = 0; i < realcount; i++) {
	    employees[i].hours = htonl(employees[i].hours);
		ssize_t employee_written = write(fd, &employees[i], sizeof(struct employee_t));
        if (employee_written != sizeof(struct employee_t)) {
            perror("write employee");
            printf("%d", employee_written);
            return STATUS_ERROR;
        }
	}

	return STATUS_SUCCESS;
}	

int validate_db_header(int fd, struct dbheader_t **headerOut) {
    if (fd < 0) {
        printf("Got a bad FD from the user\n");
        return STATUS_ERROR;
    }

 	struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if (header == -1) {
        printf("Calloc failed to create db header\n");
        return STATUS_ERROR;
    }
    if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
        perror("read");
        free(header);
        return STATUS_ERROR;
    }

    header->version = ntohs(header->version);
    header->count = ntohs(header->count);
    header->magic = ntohl(header->magic);
    header->filesize = ntohl(header->filesize);

    if (header->magic != HEADER_MAGIC) {
		printf("Impromper header magic\n");
		free(header);
		return -1;
	}

    if (header->version != HEADER_VERSION) {
        printf("Improper header version\n");
        free(header);
        return -1;
    }

    struct stat dbstat = {0};
    fstat(fd, &dbstat);

    if (header->filesize != dbstat.st_size) {
        printf("Header size not matching\n");
        free(header);
        return -1;
    }

    *headerOut = header;

    return STATUS_SUCCESS;
}

int create_db_header(int fd, struct dbheader_t **headerOut) {
	struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if (header == -1) {
        printf("Calloc failed to create db header\n");
        return STATUS_ERROR;
    }

    header->version = 1;
    header->count = 0;
    header->magic = HEADER_MAGIC;
    header->filesize = sizeof(struct dbheader_t);

    *headerOut = header;

    return STATUS_SUCCESS;
}

int update_employee_hours(struct employee_t *employees, struct dbheader_t *dbhdr, char *name, char *hours) {
    int i = 0;
    int found_index = -1;
    for (; i < dbhdr->count; i++) {
        if (strcmp(name, employees[i].name) == 0) {
            found_index = i;
        }
	}

    if (found_index == -1) {
        printf("No employee with name %s in the database\n", name);
        return STATUS_ERROR;
    }

    employees[found_index].hours = atoi(hours);

    printf("Updated %s hours to %s\n", employees[found_index].name, hours);

    return STATUS_SUCCESS;
}

int remove_employee_by_name(struct employee_t *employees, struct dbheader_t *dbhdr, char *name) {
    int found_index = -1;

    // Find the employee to remove
    for (int i = 0; i < dbhdr->count; i++) {
        if (strcmp(name, employees[i].name) == 0) {
            found_index = i;
            break;
        }
    }

    if (found_index == -1) {
        printf("No employee with name %s in the database\n", name);
        return STATUS_ERROR;
    }

    // Shift all employees after the found one to the left
    for (int i = found_index; i < dbhdr->count - 1; i++) {
        employees[i] = employees[i + 1];
    }

    // Resize the employees array
    struct employee_t *updated_employees = realloc(employees, (dbhdr->count - 1) * sizeof(struct employee_t));
    if (updated_employees == NULL && dbhdr->count > 1) {
        printf("Realloc failed\n");
        return STATUS_ERROR;
    }

    dbhdr->count--;

    employees = updated_employees;

    return STATUS_SUCCESS;
}


