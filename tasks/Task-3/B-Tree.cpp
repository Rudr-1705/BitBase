#include <iostream>
using namespace std;

class BTreeNode
{
    int t; // Max number of keys
    int *keys;
    BTreeNode **children;
    int n; // Current number of keys
    bool isLeaf;

public:
    BTreeNode(int t, bool isLeaf)
    {
        this->t = t;
        this->isLeaf = isLeaf;

        this->keys = new int[2 * t + 1];
        this->children = new BTreeNode *[2 * t];

        this->n = 0;
    }

    void insertNonFull(int k)
    {
        int i = n - 1;

        if (isLeaf)
        {
            while (i >= 0 && keys[i] > k)
            {
                keys[i + 1] = keys[i];
                i--;
            }

            keys[i + 1] = k;
            n++;
        }
        else
        {
            while (i >= 0 && keys[i] > k)
            {
                i--;
            }

            if (children[i + 1]->n == 2 * t - 1)
            {
                splitChild(i + 1, children[i + 1]);

                if (keys[i + 1] < k)
                    i++;
            }

            children[i + 1]->insertNonFull(k);
        }
    }

    void splitChild(int idx, BTreeNode *tar)
    {
        BTreeNode *newNode = new BTreeNode(tar->t, tar->isLeaf);

        // shift last t - 1 keys to the newNode. i.e [t, 2t - 2];
        newNode->n = t - 1;

        for (int j = 0; j < t - 1; j++)
            newNode->keys[j] = tar->keys[j + t];

        // shift last t children to the newNode. i.e [t, 2t - 1]
        if (!tar->isLeaf)
        {
            for (int j = 0; j < t; j++)
            {
                newNode->children[j] = tar->children[j + t];
            }
        }

        // remaining set of indices is [0, t - 1]
        // key[t - 1] will be promoted to current node
        // Therefore only keys[0, t - 2] will be left in tar, which are t - 1 keys
        tar->n = t - 1;

        // pushing children forward in the current node to make space for the new child
        for (int j = n; j >= idx + 1; j--)
            children[j + 1] = children[j];

        // insert the newChild in the current node
        children[idx + 1] = newNode;

        // move the current keys one step forward
        for (int j = n - 1; j >= idx; j--)
            keys[j + 1] = keys[j];

        // add key[t - 1] in index i
        keys[idx] = tar->keys[t - 1];

        // increment the number of keys in this node
        n++;
    }

    void traverse()
    {
        for (int i = 0; i < n; i++)
        {
            if (!isLeaf)
            {
                children[i]->traverse();
            }

            cout << keys[i] << " ";
        }

        if (!isLeaf)
            children[n]->traverse();
    }

    BTreeNode *search(int k)
    {
        int i = 0;

        while (i < n && keys[i] < k)
        {
            i++;
        }

        if (keys[i] == k)
            return this;

        if (isLeaf)
            return NULL;

        return children[i]->search(k);
    }

    friend class BTree;
};

class BTree
{
    BTreeNode *root;
    int t;

public:
    BTree(int t) : t(t)
    {
        root = NULL;
    }

    void traverse()
    {
        if (!root)
            root->traverse();
    }

    BTreeNode *search(int k)
    {
        if (!root)
            return root->search(k);
    }

    void insert(int k)
    {
        if (!root)
        {
            root = new BTreeNode(t, true);
            root->keys[0] = k;
            root->n = 1;
        }
        else
        {
            if (root->n == 2 * t - 1)
            {
                BTreeNode *newRoot = new BTreeNode(t, false);
                newRoot->children[0] = root;
                newRoot->splitChild(0, root);

                int i = 0;
                if (newRoot->keys[0] < k)
                    i++;
                newRoot->children[i]->insertNonFull(k);

                root = newRoot;
            }
            else
                root->insertNonFull(k);
        }
    }
};

int main()
{
    BTree t(3); // A B-Tree with minimum degree 3
    t.insert(10);
    t.insert(20);
    t.insert(5);
    t.insert(6);
    t.insert(12);
    t.insert(30);
    t.insert(7);
    t.insert(17);

    cout << "Traversal of the constructed tree is ";
    t.traverse();

    int k = 6;
    (t.search(k) != NULL) ? cout << "\nPresent" : cout << "\nNot Present";

    k = 15;
    (t.search(k) != NULL) ? cout << "\nPresent" : cout << "\nNot Present";

    return 0;
}