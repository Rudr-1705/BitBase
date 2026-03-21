#include <bits/stdc++.h>
using namespace std;

class BPlusNode
{
public:
    bool isLeaf;
    int n;     // number of keys
    int order; // max children
    vector<int> keys;
    vector<BPlusNode *> children;
    BPlusNode *next;

    BPlusNode(int order, bool isLeaf)
        : isLeaf(isLeaf), n(0), order(order),
          keys(order), children(order + 1, nullptr),
          next(nullptr) {}
};

class BPlusTree
{
private:
    BPlusNode *root;
    int order;

private:
    void insertIntoLeaf(BPlusNode *leaf, int key)
    {
        int i = leaf->n - 1;
        while (i >= 0 && leaf->keys[i] > key)
        {
            leaf->keys[i + 1] = leaf->keys[i];
            i--;
        }
        leaf->keys[i + 1] = key;
        leaf->n++;
    }

    BPlusNode *splitLeaf(BPlusNode *leaf, int &upKey)
    {
        int mid = (order + 1) / 2;

        BPlusNode *newLeaf = new BPlusNode(order, true);

        newLeaf->n = leaf->n - mid;
        for (int i = 0; i < newLeaf->n; i++)
            newLeaf->keys[i] = leaf->keys[mid + i];

        leaf->n = mid;

        newLeaf->next = leaf->next;
        leaf->next = newLeaf;

        upKey = newLeaf->keys[0]; // the first key of the 2nd part will be promoted up -> right biased
        return newLeaf;
    }

    void insertIntoInternal(BPlusNode *node, int key, BPlusNode *rightChild)
    {
        int i = node->n - 1;
        while (i >= 0 && node->keys[i] > key)
        {
            node->keys[i + 1] = node->keys[i];
            node->children[i + 2] = node->children[i + 1];
            i--;
        }
        node->keys[i + 1] = key;
        node->children[i + 2] = rightChild;
        node->n++;
    }

    BPlusNode *splitInternal(BPlusNode *node, int &upKey)
    {
        int mid = node->n / 2;

        BPlusNode *newNode = new BPlusNode(order, false);

        upKey = node->keys[mid];

        newNode->n = node->n - mid - 1;
        for (int i = 0; i < newNode->n; i++)
            newNode->keys[i] = node->keys[mid + 1 + i];

        for (int i = 0; i <= newNode->n; i++)
            newNode->children[i] = node->children[mid + 1 + i];

        node->n = mid;
        return newNode;
    }

    BPlusNode *insertRecursive(BPlusNode *node, int key, int &upKey)
    {
        if (node->isLeaf)
        {
            insertIntoLeaf(node, key);

            if (node->n < order)
                return nullptr;

            return splitLeaf(node, upKey);
        }

        int i = 0;
        while (i < node->n && key >= node->keys[i])
            i++;

        int newKey;
        BPlusNode *newChild =
            insertRecursive(node->children[i], key, newKey);

        if (!newChild)
            return nullptr;

        insertIntoInternal(node, newKey, newChild);

        if (node->n < order)
            return nullptr;

        return splitInternal(node, upKey);
    }

public:
    BPlusTree(int order) : root(nullptr), order(order) {}

    void insert(int key)
    {
        if (!root)
        {
            root = new BPlusNode(order, true);
            root->keys[0] = key;
            root->n = 1;
            return;
        }

        int upKey;
        BPlusNode *newChild = insertRecursive(root, key, upKey);

        if (!newChild)
            return;

        BPlusNode *newRoot = new BPlusNode(order, false);
        newRoot->keys[0] = upKey;
        newRoot->children[0] = root;
        newRoot->children[1] = newChild;
        newRoot->n = 1;
        root = newRoot;
    }

    void printLeaves()
    {
        if (!root)
            return;

        BPlusNode *cur = root;
        while (!cur->isLeaf)
            cur = cur->children[0];

        while (cur)
        {
            for (int i = 0; i < cur->n; i++)
                cout << cur->keys[i] << " ";
            cur = cur->next;
        }
        cout << "\n";
    }
};

int main()
{
    BPlusTree tree(4); // order = 4

    vector<int> vals = {10, 20, 5, 6, 12, 30, 7, 17};
    for (int x : vals)
        tree.insert(x);

    tree.printLeaves();
    return 0;
}
