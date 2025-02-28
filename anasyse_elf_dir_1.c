#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h>
#include <elf.h>

// Функция для вывода ошибок
void fatal(const char *msg) {
    fprintf(stderr, "Ошибка: %s\n", msg);
    exit(1);
}

// Функция для проверки, является ли файл ELF
int is_elf_file(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) return 0;
    char magic[4];
    if (read(fd, magic, 4) != 4) {
        close(fd);
        return 0;
    }

    close(fd);
    return memcmp(magic, ELFMAG, SELFMAG) == 0;
}

// Функция для анализа ELF-файла
void analyze_elf_file(const char *filename) {
    int fd;
    struct stat st;
    Elf64_Ehdr *ehdr;
    Elf64_Shdr *shdr;
    Elf64_Phdr *phdr;
    char *shstrtab = NULL;
    int i;

    // Открываем файл
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return;
    }

    // Получаем размер файла
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return;
    }
    // Отображаем файл в память
    void *data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return;
    }

    // Заголовок ELF
    ehdr = (Elf64_Ehdr *)data;

    // Проверяем, что это ELF-файл
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "%s: Это не ELF-файл\n", filename);
        munmap(data, st.st_size);
        close(fd);
        return;
    }
    // Получаем таблицу секций
    shdr = (Elf64_Shdr *)(data + ehdr->e_shoff);
    // Получаем таблицу строк секций
    //shstrtab = (char *)malloc(shdr->sh_size);
    //memcpy(&shstrtab, (char *)data + shdr->sh_offset, shdr->sh_size);
    shstrtab = (char *)(data + shdr[ehdr->e_shstrndx].sh_offset);
    
    // Получаем таблицу программных заголовков
    phdr = (Elf64_Phdr *)(data + ehdr->e_phoff);
    // 1. Проверяем, используется ли Position Independent Code (PIC)
    int has_pic = 0;
    for (i = 0; i < ehdr->e_shnum; i++) {
        if (strcmp(&shstrtab[shdr[i].sh_name], ".got.plt") == 0) {
            has_pic = 1;
            break;
        }
    }
    if (has_pic == 1){
    printf("Анализ файла: %s\n", filename);
    printf(" %s:\n", has_pic ? "[+]" : "[-]");
}
    //free(shstrtab);
    // Освобождаем ресурсы
    munmap(data, st.st_size);
    close(fd);
}

// Функция для рекурсивного обхода каталога
void scan_directory(const char *path) {
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(path))) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            // Пропускаем текущий и родительский каталоги
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            // Рекурсивно обходим подкаталог
            char new_path[1024];
            snprintf(new_path, sizeof(new_path), "%s/%s", path, entry->d_name);
            scan_directory(new_path);
        } else if (entry->d_type == DT_REG) {
            // Проверяем, является ли файл ELF
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
            if (is_elf_file(full_path)) {
                analyze_elf_file(full_path);
            }
        }
    }

    closedir(dir);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fatal("Использование: ./elf_scanner <каталог>");
    }

    const char *directory = argv[1];
    scan_directory(directory);

    return 0;
}
