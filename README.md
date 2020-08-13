# proton_json_parser

This project is using to learning Clang.

### clang 内存泄露检测工具
Linux Centos `sudo yum install valgrind`
`valgrind --leak-check=full ./main` 

```text
==20921== Memcheck, a memory error detector
==20921== Copyright (C) 2002-2017, and GNU GPL'd, by Julian Seward et al.
==20921== Using Valgrind-3.15.0 and LibVEX; rerun with -h for copyright info
==20921== Command: ./main
==20921== 
84/84 (100.00%) passed
==20921== 
==20921== HEAP SUMMARY:
==20921==     in use at exit: 0 bytes in 0 blocks
==20921==   total heap usage: 10 allocs, 10 frees, 548 bytes allocated
==20921== 
==20921== All heap blocks were freed -- no leaks are possible
==20921== 
==20921== For lists of detected and suppressed errors, rerun with: -s
==20921== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```
