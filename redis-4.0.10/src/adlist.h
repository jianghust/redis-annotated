/* adlist.h - A generic doubly linked list implementation
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

#ifndef __ADLIST_H__
#define __ADLIST_H__

/* Node, List, and Iterator are the only data structures used currently. */

typedef struct listNode {
	//前一个节点
    struct listNode *prev;
	//后一个节点
    struct listNode *next;
	//节点值
    void *value;
} listNode;

//链表迭代器,用来遍历链表
typedef struct listIter {
	//当前节点
    listNode *next;
	//迭代方向
    int direction;
} listIter;

typedef struct list {
	//头节点指针
    listNode *head;
	//尾节点指针
    listNode *tail;
	//节点值复制函数
    void *(*dup)(void *ptr);
	//节点值释放函数
    void (*free)(void *ptr);
	//节点值对比函数
    int (*match)(void *ptr, void *key);
	//链表所包含节点的数量
    unsigned long len;
} list;

/* Functions implemented as macros */
#define listLength(l) ((l)->len)
#define listFirst(l) ((l)->head)
#define listLast(l) ((l)->tail)
#define listPrevNode(n) ((n)->prev)
#define listNextNode(n) ((n)->next)
#define listNodeValue(n) ((n)->value)

#define listSetDupMethod(l,m) ((l)->dup = (m))
#define listSetFreeMethod(l,m) ((l)->free = (m))
#define listSetMatchMethod(l,m) ((l)->match = (m))

#define listGetDupMethod(l) ((l)->dup)
#define listGetFree(l) ((l)->free)
#define listGetMatchMethod(l) ((l)->match)

/* Prototypes */
//创建一个链表
list *listCreate(void);
//释放链表,包括链表下的所有节点和链表本身
void listRelease(list *list);
//释放链表下的所有节点
void listEmpty(list *list);
//链表头部新增一个节点
list *listAddNodeHead(list *list, void *value);
//链表尾部新增一个节点
list *listAddNodeTail(list *list, void *value);
//往链表中出入一个节点
list *listInsertNode(list *list, listNode *old_node, void *value, int after);
//链表中删除一个节点
void listDelNode(list *list, listNode *node);
listIter *listGetIterator(list *list, int direction);
//返回给定节点的后置节点
listNode *listNext(listIter *iter);
void listReleaseIterator(listIter *iter);
//复制一个给定链表的副本
list *listDup(list *orig);
//查找并且返回链表中value包含key值的节点
listNode *listSearchKey(list *list, void *key);
listNode *listIndex(list *list, long index);
//迭代器重置到表头
void listRewind(list *list, listIter *li);
//迭代器重置到表尾
void listRewindTail(list *list, listIter *li);
//将链表尾节点移动为头结点
void listRotate(list *list);
//将两个链表合并
void listJoin(list *l, list *o);

/* Directions for iterators */
//从头到尾遍历
#define AL_START_HEAD 0
//从未到头遍历
#define AL_START_TAIL 1

#endif /* __ADLIST_H__ */
