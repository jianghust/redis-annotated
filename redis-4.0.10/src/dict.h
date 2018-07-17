/* Hash Tables Implementation.
 *
 * This file implements in-memory hash tables with insert/del/replace/find/
 * get-random-element operations. Hash tables will auto-resize if needed
 * tables of power of two in size are used, collisions are handled by
 * chaining. See the source code for more information... :)
 *
 * Copyright (c) 2006-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>

#ifndef __DICT_H
#define __DICT_H

#define DICT_OK 0
#define DICT_ERR 1

/* Unused arguments generate annoying warnings... */
#define DICT_NOTUSED(V) ((void) V)

//hash表节点
typedef struct dictEntry {
	//建
    void *key;
	//值
    union {
        void *val;
        uint64_t u64;
        int64_t s64;
        double d;
    } v;
	//下一个hash节点
    struct dictEntry *next;
} dictEntry;

typedef struct dictType {
	//计算hash值的函数
    uint64_t (*hashFunction)(const void *key);
	//复制建的函数
    void *(*keyDup)(void *privdata, const void *key);
	//复制值的函数
    void *(*valDup)(void *privdata, const void *obj);
	//对比建的函数
    int (*keyCompare)(void *privdata, const void *key1, const void *key2);
	//销毁建的函数
    void (*keyDestructor)(void *privdata, void *key);
	//销毁值的函数
    void (*valDestructor)(void *privdata, void *obj);
} dictType;

/* This is our hash table structure. Every dictionary has two of this as we
 * implement incremental rehashing, for the old to the new table. */
//hash表
typedef struct dictht {
	//hash表数组
    dictEntry **table;
	//hash表大小
    unsigned long size;
	//hash大小的掩码，用于计算索引值，总是等于size-1
    unsigned long sizemask;
	//改hash已有节点的数量
    unsigned long used;
} dictht;
//字典结构
typedef struct dict {
	//类型特定函数,redis会为用途不同的字典设置不同的类型特定函数
    dictType *type;
	//私有数据,保存了需要传递给哪些类型特定函数的可选参数
    void *privdata;
	//hash表,两个数组，一个表示hash表，另外一个是rehash表
    dictht ht[2];
	//rehash当前索引值,记录了rehash目前的进度,当rehash不在进行时,值为-1
    long rehashidx; /* rehashing not in progress if rehashidx == -1 */
	//当前运行的迭代器数量
    unsigned long iterators; /* number of iterators currently running */
} dict;

/* If safe is set to 1 this is a safe iterator, that means, you can call
 * dictAdd, dictFind, and other functions against the dictionary even while
 * iterating. Otherwise it is a non safe iterator, and only dictNext()
 * should be called while iterating. */
//字典迭代器
typedef struct dictIterator {
	//字典
    dict *d;
	//当前索引值
    long index;
    int table, safe;
    dictEntry *entry, *nextEntry;
    /* unsafe iterator fingerprint for misuse detection. */
    long long fingerprint;
} dictIterator;

//字典扫描方法
typedef void (dictScanFunction)(void *privdata, const dictEntry *de);
typedef void (dictScanBucketFunction)(void *privdata, dictEntry **bucketref);

/* This is the initial size of every hash table */
#define DICT_HT_INITIAL_SIZE     4

/* ------------------------------- Macros ------------------------------------*/
#define dictFreeVal(d, entry) \
    if ((d)->type->valDestructor) \
        (d)->type->valDestructor((d)->privdata, (entry)->v.val)

#define dictSetVal(d, entry, _val_) do { \
    if ((d)->type->valDup) \
        (entry)->v.val = (d)->type->valDup((d)->privdata, _val_); \
    else \
        (entry)->v.val = (_val_); \
} while(0)

#define dictSetSignedIntegerVal(entry, _val_) \
    do { (entry)->v.s64 = _val_; } while(0)

#define dictSetUnsignedIntegerVal(entry, _val_) \
    do { (entry)->v.u64 = _val_; } while(0)

#define dictSetDoubleVal(entry, _val_) \
    do { (entry)->v.d = _val_; } while(0)

#define dictFreeKey(d, entry) \
    if ((d)->type->keyDestructor) \
        (d)->type->keyDestructor((d)->privdata, (entry)->key)

#define dictSetKey(d, entry, _key_) do { \
    if ((d)->type->keyDup) \
        (entry)->key = (d)->type->keyDup((d)->privdata, _key_); \
    else \
        (entry)->key = (_key_); \
} while(0)

#define dictCompareKeys(d, key1, key2) \
    (((d)->type->keyCompare) ? \
        (d)->type->keyCompare((d)->privdata, key1, key2) : \
        (key1) == (key2))

#define dictHashKey(d, key) (d)->type->hashFunction(key)
#define dictGetKey(he) ((he)->key)
#define dictGetVal(he) ((he)->v.val)
#define dictGetSignedIntegerVal(he) ((he)->v.s64)
#define dictGetUnsignedIntegerVal(he) ((he)->v.u64)
#define dictGetDoubleVal(he) ((he)->v.d)
#define dictSlots(d) ((d)->ht[0].size+(d)->ht[1].size)
#define dictSize(d) ((d)->ht[0].used+(d)->ht[1].used)
#define dictIsRehashing(d) ((d)->rehashidx != -1)

/**
 * 1、当dict被当做数据库底层的实现，或者hash键底层实现，使用MurmurHash2算法来计算键的hash值
 * 2、hash使用链地址法来解决冲突，被分配到同一个索引上的多个键值对会连接成一个单向的hash表
 * 3、对hash表进行扩缩容时候，是渐进式的
 * 4、负载因子：load_factor = ht[0].used/ht[0].size 得出来
 * 5、扩容操作
 *	(1) 没有执行BGSAVE或者BGREWRITEAOF时候，负载因子大于等于1
 *	(2) 正在执行BGSAVE或者BGREWRITEAOF时候，负载因子大于等于5
 * 6、缩容操作
 *	当负载因子小于0.1的时候，执行缩容操作
 *
 *
 */
/* API */
//创建一个新的字典
dict *dictCreate(dictType *type, void *privDataPtr);
//扩展一个字典的长度
int dictExpand(dict *d, unsigned long size);
//将给定的键值对添加到字典中
int dictAdd(dict *d, void *key, void *val);
//插入一个节点
dictEntry *dictAddRaw(dict *d, void *key, dictEntry **existing);
//添加或查找一个节点，并将这个节点返回
dictEntry *dictAddOrFind(dict *d, void *key);
//将给定的键值对添加到字典中，如果已经存在，则直接替换掉
int dictReplace(dict *d, void *key, void *val);
//从字典中删除给定的键值对
int dictDelete(dict *d, const void *key);
//删除一个节点但保留节点的内存
dictEntry *dictUnlink(dict *ht, const void *key);
//释放节点内存,dictUnlink调用之后再调用
void dictFreeUnlinkedEntry(dict *d, dictEntry *he);
//释放给定字典，以及字典中所有的键值对
void dictRelease(dict *d);
//查找一个节点
dictEntry * dictFind(dict *d, const void *key);
//获取某个节点值
void *dictFetchValue(dict *d, const void *key);
//调整hash大小
int dictResize(dict *d);
//获取迭代器
dictIterator *dictGetIterator(dict *d);
//获取字典的安全迭代器
dictIterator *dictGetSafeIterator(dict *d);
//根据字典迭代器获取下一个字典
dictEntry *dictNext(dictIterator *iter);
//销毁一个迭代器
void dictReleaseIterator(dictIterator *iter);
//返回字典中一个随机非空节点，是完全随机，有两次随机过程，第一次在ht[0]和ht[1]中找一个随机的非空桶，第二次在找到的非空桶中获取一个随机的元素
dictEntry *dictGetRandomKey(dict *d);
//返回一个字典中的若干随机节点，是不完全的随机，先用随机数找到一个桶，若这个桶是非空的，取这个桶中的所有节点，若是空的，继续下一个桶，连续遇到5个空桶的话重置随机数，找到给定数量的节点或者达到阈值，结束
unsigned int dictGetSomeKeys(dict *d, dictEntry **des, unsigned int count);
////获取当前字典状态
void dictGetStats(char *buf, size_t bufsize, dict *d);
//使用key计算hash index，hash算法用siphash
uint64_t dictGenHashFunction(const void *key, int len);
//字符串类型的case hash计算index
uint64_t dictGenCaseHashFunction(const unsigned char *buf, int len);
//清空一个字典，包含清空2个hash表和迭代器
void dictEmpty(dict *d, void(callback)(void*));
//设置允许扩张表
void dictEnableResize(void);
//设置不允许扩张表
void dictDisableResize(void);
//每次rehash n个节点,最多只允许访问ht[0]中的10 * n 个桶,否则就散个数不够n个节点也终止
int dictRehash(dict *d, int n);
//循环每次reash 100个节点，到ms时间之后终止
int dictRehashMilliseconds(dict *d, int ms);
//将提供的hash种子保存到全局变量
void dictSetHashFunctionSeed(uint8_t *seed);
//获取hash算法种子
uint8_t *dictGetHashFunctionSeed(void);
//遍历一个字典
unsigned long dictScan(dict *d, unsigned long v, dictScanFunction *fn, dictScanBucketFunction *bucketfn, void *privdata);
//使用key计算hash值
uint64_t dictGetHash(dict *d, const void *key);
//使用key和key的hash值在字典中查找一个节点
dictEntry **dictFindEntryRefByPtrAndHash(dict *d, const void *oldptr, uint64_t hash);

/* Hash table types */
extern dictType dictTypeHeapStringCopyKey;
extern dictType dictTypeHeapStrings;
extern dictType dictTypeHeapStringCopyKeyValue;

#endif /* __DICT_H */
