# 2020 OS Project 1

> B05902040 資工四 宋易軒

## 設計

* 由一個負責scheduling的process，負責決定該跑哪個process
* 使用兩顆CPU，利用**sched_setaffinity**做設定，一個給負責scheduling的parent，一個給fork出來的child process，避免block住parent的排程工作
* 在排程之前先對child process依據ready time排序，有助FIFO與RR方便找尋下一個要跑的process
* 當一支process第一次獲得執行的priority時，透過**fork**來建立process，並在該process計時，若execution time到了便結束
* 一支process能否真正的在CPU執行的priority，透過**sched_setscheduler**，利用**SCHED_IDLE**以及**SCHED_OTHER**兩個參數，這兩個參數分別對應到要被block以及要被執行的process
* Schedule policy
  * PSJF: 每個時間點都檢查各個process剩餘的執行時間，並執行最短的
  * SJF: 每當CPU閒置或有process結束時，會去檢查各個process剩餘的執行時間，並執行最短的
  * FIFO: 每當CPU閒置或有process結束時，會從一開始照ready time排序的processes中，挑下一個執行
  * RR:每當CPU閒置或有process結束時，會從一開始照ready time排序的processes中，挑下一個執行。在每個time quantum時間到時，會循環式的尋找下一個process來執行
* syscall: 基於printk跟getnstimeofday，建立自己的system function
  * my_get_time(): 負責call getnstimeofday，並把當時的秒數紀錄起來
  * my_print_msg(): call printk，把函式輸入經由printk印出至dmesg
* parent process在生成process以及process結束時會記錄當前時間，最後印至dmesg

## 核心版本

linux-4.14.25



## 比較

經過TIME_MEASUREMENT轉換後所得單位時間，再去轉換跑出來的實驗結果，可發現跟理論值有所差距，但對於單次的scheduling所計算的單位時間，相對的執行時間與順序大致吻合。

而因為scheduler在user space進行排程，需要做計算並處理相關資料，無法最即時的改變process執行狀況，每次process在執行迴圈進行Unit time計算所費時間也不一定完全相同，亦會影響結果，加上虛擬環境的不確定性，造成了誤差。



下表為實驗值

```
---------- FIFO 1 ----------
P1 -> start from 0 to 466
P2 -> start from 466 to 828
P3 -> start from 828 to 1185
P4 -> start from 1185 to 1749
P5 -> start from 1749 to 2115
---------- FIFO 2 ----------
P1 -> start from 0 to 67802
P2 -> start from 67802 to 69990
P3 -> start from 69990 to 70427
P4 -> start from 70427 to 70865
---------- FIFO 3 ----------
P1 -> start from 0 to 3948
P2 -> start from 3948 to 7416
P3 -> start from 7416 to 10710
P4 -> start from 10710 to 11977
P5 -> start from 11977 to 12988
P6 -> start from 12988 to 13907
P7 -> start from 13907 to 18151
---------- FIFO 4 ----------
P1 -> start from 0 to 2130
P2 -> start from 2130 to 2451
P3 -> start from 2451 to 2557
P4 -> start from 2557 to 2787
---------- FIFO 5 ----------
P1 -> start from 0 to 7403
P2 -> start from 7403 to 11586
P3 -> start from 11586 to 15048
P4 -> start from 15048 to 16116
P5 -> start from 16116 to 17051
P6 -> start from 17051 to 17545
P7 -> start from 17546 to 19722
---------- RR 1 ----------
P1 -> start from 0 to 402
P2 -> start from 402 to 706
P3 -> start from 706 to 1005
P4 -> start from 1005 to 1260
P5 -> start from 1260 to 1472
---------- RR 2 ----------
P1 -> start from 600 to 5105
P2 -> start from 937 to 5740
---------- RR 3 ----------
P1 -> start from 1200 to 11841
P2 -> start from 2311 to 12060
P3 -> start from 2963 to 11103
P4 -> start from 3829 to 16854
P5 -> start from 4040 to 16416
P6 -> start from 4698 to 15546
---------- RR 4 ----------
P1 -> start from 0 to 17860
P2 -> start from 496 to 15725
P3 -> start from 953 to 11227
P4 -> start from 1409 to 5876
P5 -> start from 1960 to 6483
P6 -> start from 2530 to 6996
P7 -> start from 3403 to 14234
---------- RR 5 ----------
P1 -> start from 0 to 13985
P2 -> start from 513 to 12697
P3 -> start from 1030 to 9901
P4 -> start from 1340 to 3077
P5 -> start from 1589 to 3300
P6 -> start from 2016 to 3725
P7 -> start from 2229 to 12053
---------- SJF 1 ----------
P1 -> start from 4106 to 7730
P2 -> start from 0 to 879
P3 -> start from 879 to 1954
P4 -> start from 1954 to 4106
---------- SJF 2 ----------
P1 -> start from 100 to 213
P2 -> start from 444 to 2416
P3 -> start from 213 to 444
P4 -> start from 2416 to 4125
P5 -> start from 4125 to 8849
---------- SJF 3 ----------
P1 -> start from 100 to 2082
P2 -> start from 7307 to 9839
P3 -> start from 9839 to 12900
P4 -> start from 2082 to 2095
P5 -> start from 2095 to 2108
P6 -> start from 2108 to 4600
P7 -> start from 4600 to 7307
P8 -> start from 12900 to 19470
---------- SJF 4 ----------
P1 -> start from 0 to 2828
P2 -> start from 2828 to 3489
P3 -> start from 3489 to 6582
P4 -> start from 7813 to 10163
P5 -> start from 6582 to 7813
---------- SJF 5 ----------
P1 -> start from 0 to 2352
P2 -> start from 2352 to 3009
P3 -> start from 3009 to 3495
P4 -> start from 3495 to 3790
---------- PSJF 1 ----------
P1 -> start from 0 to 15777
P2 -> start from 1025 to 10419
P3 -> start from 2262 to 7686
P4 -> start from 3535 to 5428
---------- PSJF 2 ----------
P1 -> start from 0 to 2262
P2 -> start from 858 to 1375
P3 -> start from 2262 to 5645
P4 -> start from 2680 to 3556
P5 -> start from 3556 to 4001
---------- PSJF 3 ----------
P1 -> start from 0 to 2188
P2 -> start from 311 to 565
P3 -> start from 565 to 1094
P4 -> start from 1094 to 1431
---------- PSJF 4 ----------
P1 -> start from 4840 to 9419
P2 -> start from 0 to 2149
P3 -> start from 118 to 881
P4 -> start from 2149 to 4840
---------- PSJF 5 ----------
P1 -> start from 100 to 242
P2 -> start from 511 to 5336
P3 -> start from 242 to 511
P4 -> start from 5336 to 9061
P5 -> start from 9061 to 13367

```



下表為理論值

```
---------- FIFO 1 ----------
P1 -> start from 0 to 500
P2 -> start from 500 to 1000
P3 -> start from 1000 to 1500
P4 -> start from 1500 to 2000
P5 -> start from 2000 to 2500
---------- FIFO 2 ----------
P1 -> start from 0 to 80000
P2 -> start from 80000 to 85000
P3 -> start from 85000 to 86000
P4 -> start from 86000 to 87000
---------- FIFO 3 ----------
P1 -> start from 0 to 8000
P2 -> start from 8000 to 13000
P3 -> start from 13000 to 16000
P4 -> start from 16000 to 17000
P5 -> start from 17000 to 18000
P6 -> start from 18000 to 19000
P7 -> start from 19000 to 23000
---------- FIFO 4 ----------
P1 -> start from 0 to 2000
P2 -> start from 2000 to 2500
P3 -> start from 2500 to 2700
P4 -> start from 2700 to 3200
---------- FIFO 5 ----------
P1 -> start from 0 to 8000
P2 -> start from 8000 to 13000
P3 -> start from 13000 to 16000
P4 -> start from 16000 to 17000
P5 -> start from 17000 to 18000
P6 -> start from 18000 to 19000
P7 -> start from 19000 to 23000
---------- RR 1 ----------
P1 -> start from 0 to 500
P2 -> start from 500 to 1000
P3 -> start from 1000 to 1500
P4 -> start from 1500 to 2000
P5 -> start from 2000 to 2500
---------- RR 2 ----------
P1 -> start from 600 to 8100
P2 -> start from 1100 to 9600
---------- RR 3 ----------
P1 -> start from 1200 to 20700
P2 -> start from 2400 to 19900
P3 -> start from 4400 to 18900
P4 -> start from 5900 to 31200
P5 -> start from 6900 to 30200
P6 -> start from 7900 to 28200
---------- RR 4 ----------
P1 -> start from 0 to 23000
P2 -> start from 500 to 20000
P3 -> start from 1000 to 14500
P4 -> start from 1500 to 5500
P5 -> start from 2000 to 6000
P6 -> start from 2500 to 6500
P7 -> start from 3500 to 18500
---------- RR 5 ----------
P1 -> start from 0 to 23000
P2 -> start from 500 to 20000
P3 -> start from 1000 to 14500
P4 -> start from 1500 to 5500
P5 -> start from 2000 to 6000
P6 -> start from 3000 to 7000
P7 -> start from 3500 to 18500
---------- SJF 1 ----------
P1 -> start from 7000 to 14000
P2 -> start from 0 to 2000
P3 -> start from 2000 to 3000
P4 -> start from 3000 to 7000
---------- SJF 2 ----------
P1 -> start from 100 to 200
P2 -> start from 400 to 4400
P3 -> start from 200 to 400
P4 -> start from 4400 to 8400
P5 -> start from 8400 to 15400
---------- SJF 3 ----------
P1 -> start from 100 to 3100
P2 -> start from 11120 to 16120
P3 -> start from 16120 to 23120
P4 -> start from 3100 to 3110
P5 -> start from 3110 to 3120
P6 -> start from 3120 to 7120
P7 -> start from 7120 to 11120
P8 -> start from 23120 to 32120
---------- SJF 4 ----------
P1 -> start from 0 to 3000
P2 -> start from 3000 to 4000
P3 -> start from 4000 to 8000
P4 -> start from 9000 to 11000
P5 -> start from 8000 to 9000
---------- SJF 5 ----------
P1 -> start from 0 to 2000
P2 -> start from 2000 to 2500
P3 -> start from 2500 to 3000
P4 -> start from 3000 to 3500
---------- PSJF 1 ----------
P1 -> start from 0 to 25000
P2 -> start from 1000 to 16000
P3 -> start from 2000 to 10000
P4 -> start from 3000 to 6000
---------- PSJF 2 ----------
P1 -> start from 0 to 4000
P2 -> start from 1000 to 2000
P3 -> start from 4000 to 11000
P4 -> start from 5000 to 7000
P5 -> start from 7000 to 8000
---------- PSJF 3 ----------
P1 -> start from 0 to 3500
P2 -> start from 500 to 1000
P3 -> start from 1000 to 1500
P4 -> start from 1500 to 2000
---------- PSJF 4 ----------
P1 -> start from 7000 to 14000
P2 -> start from 0 to 3000
P3 -> start from 100 to 1100
P4 -> start from 3000 to 7000
---------- PSJF 5 ----------
P1 -> start from 100 to 200
P2 -> start from 400 to 4400
P3 -> start from 200 to 400
P4 -> start from 4400 to 8400
P5 -> start from 8400 to 15400
```

