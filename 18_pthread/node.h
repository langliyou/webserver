#ifndef NODE_H
#define NODE_H

#include <stdio.h>
#include <stdlib.h>
typedef struct Node {  
    int val;  
    struct Node *next;  
} Node;  
  
// 创建一个具有默认值的Node  
Node* node_() {  
    Node* newNode = (Node*)malloc(sizeof(Node));  
    if (newNode != NULL) {  
        newNode->val = 0;  
        newNode->next = NULL;  
    }  
    return newNode;  
}  
  
// 创建一个具有指定val的Node  
Node* node_v(int val) {  
    Node* newNode = (Node*)malloc(sizeof(Node));  
    if (newNode != NULL) {  
        newNode->val = val;  
        newNode->next = NULL;  
    }  
    return newNode;  
}  
  
// 创建一个具有指定val和next的Node  
Node* node_v_n(int val, Node* next) {  
    Node* newNode = (Node*)malloc(sizeof(Node));  
    if (newNode != NULL) {  
        newNode->val = val;  
        newNode->next = next;  
    }  
    return newNode;  
} 
#endif