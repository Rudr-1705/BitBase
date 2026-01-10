#include <iostream>
using namespace std;

enum Color
{
    RED,
    BLACK
};

class Node
{
public:
    int key, val;
    Color color;
    Node *left, *right, *parent;

    Node(int k, int v)
        : key(k), val(v), color(RED),
          left(nullptr), right(nullptr), parent(nullptr) {}
};

Node *root = nullptr;

void leftRotate(Node *x)
{
    Node *y = x->right;
    x->right = y->left;

    if (y->left)
        y->left->parent = x;

    y->parent = x->parent;

    if (!x->parent)
        root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;

    y->left = x;
    x->parent = y;
}

void rightRotate(Node *y)
{
    Node *x = y->left;
    y->left = x->right;

    if (x->right)
        x->right->parent = y;

    x->parent = y->parent;

    if (!y->parent)
        root = x;
    else if (y == y->parent->left)
        y->parent->left = x;
    else
        y->parent->right = x;

    x->right = y;
    y->parent = x;
}

void fixInsert(Node *z)
{
    while (z->parent && z->parent->color == RED)
    {
        Node *gp = z->parent->parent;

        if (z->parent == gp->left)
        {
            Node *uncle = gp->right;

            if (uncle && uncle->color == RED)
            {
                z->parent->color = BLACK;
                uncle->color = BLACK;
                gp->color = RED;
                z = gp;
            }
            else
            {
                if (z == z->parent->right)
                {
                    z = z->parent;
                    leftRotate(z);
                }
                z->parent->color = BLACK;
                gp->color = RED;
                rightRotate(gp);
            }
        }
        else
        {
            Node *uncle = gp->left;

            if (uncle && uncle->color == RED)
            {
                z->parent->color = BLACK;
                uncle->color = BLACK;
                gp->color = RED;
                z = gp;
            }
            else
            {
                if (z == z->parent->left)
                {
                    z = z->parent;
                    rightRotate(z);
                }
                z->parent->color = BLACK;
                gp->color = RED;
                leftRotate(gp);
            }
        }
    }
    root->color = BLACK;
}

void insert(int key, int val)
{
    Node *parent = nullptr;
    Node *curr = root;

    while (curr)
    {
        parent = curr;
        if (key < curr->key)
            curr = curr->left;
        else if (key > curr->key)
            curr = curr->right;
        else
        {
            curr->val = val;
            cout << "Updated key " << key << endl;
            return;
        }
    }

    Node *node = new Node(key, val);
    node->parent = parent;

    if (!parent)
        root = node;
    else if (key < parent->key)
        parent->left = node;
    else
        parent->right = node;

    fixInsert(node);
    cout << "Inserted key " << key << endl;
}

Node *findNode(Node *node, int key)
{
    if (!node)
        return nullptr;
    if (key < node->key)
        return findNode(node->left, key);
    if (key > node->key)
        return findNode(node->right, key);
    return node;
}

void find(int key)
{
    Node *res = findNode(root, key);
    if (res)
        cout << res->val << endl;
    else
        cout << -1 << endl;
}

void fixDelete(Node *child)
{
    while (child != root && (!child || child->color == BLACK))
    {
        Node *parent = child->parent;

        if (child == parent->left)
        {
            Node *sibling = parent->right;

            if (sibling && sibling->color == RED)
            {
                sibling->color = BLACK;
                parent->color = RED;
                leftRotate(parent);
                sibling = parent->right;
            }

            if ((!sibling->left || sibling->left->color == BLACK) &&
                (!sibling->right || sibling->right->color == BLACK))
            {
                sibling->color = RED;
                child = parent;
            }
            else
            {
                if (!sibling->right || sibling->right->color == BLACK)
                {
                    if (sibling->left)
                        sibling->left->color = BLACK;
                    sibling->color = RED;
                    rightRotate(sibling);
                    sibling = parent->right;
                }

                sibling->color = parent->color;
                parent->color = BLACK;
                if (sibling->right)
                    sibling->right->color = BLACK;
                leftRotate(parent);
                child = root;
            }
        }
        else
        {
            Node *sibling = parent->left;

            if (sibling && sibling->color == RED)
            {
                sibling->color = BLACK;
                parent->color = RED;
                rightRotate(parent);
                sibling = parent->left;
            }

            if ((!sibling->left || sibling->left->color == BLACK) &&
                (!sibling->right || sibling->right->color == BLACK))
            {
                sibling->color = RED;
                child = parent;
            }
            else
            {
                if (!sibling->left || sibling->left->color == BLACK)
                {
                    if (sibling->right)
                        sibling->right->color = BLACK;
                    sibling->color = RED;
                    leftRotate(sibling);
                    sibling = parent->left;
                }

                sibling->color = parent->color;
                parent->color = BLACK;
                if (sibling->left)
                    sibling->left->color = BLACK;
                rightRotate(parent);
                child = root;
            }
        }
    }

    if (child)
        child->color = BLACK;
}

void replace(Node *u, Node *v)
{
    if (!u->parent)
        root = v;
    else if (u == u->parent->left)
        u->parent->left = v;
    else
        u->parent->right = v;

    if (v)
        v->parent = u->parent;
}

Node *findIS(Node *node)
{
    while (node->left)
        node = node->left;
    return node;
}

void deleteKey(int key)
{
    Node *delNode = findNode(root, key);
    if (!delNode)
    {
        cout << "Key " << key << " Not found" << endl;
        return;
    }

    Node *nodeRemoved = delNode;
    Node *child = nullptr;
    Color origColor = nodeRemoved->color;

    // Case 1: no left child
    if (!delNode->left)
    {
        child = delNode->right;
        replace(delNode, delNode->right);
    }
    // Case 2: no right child
    else if (!delNode->right)
    {
        child = delNode->left;
        replace(delNode, delNode->left);
    }
    // Case 3: two children
    else
    {
        Node *IS = findIS(delNode->right);
        nodeRemoved = IS;
        origColor = IS->color;
        child = IS->right;

        if (IS->parent != delNode)
        {
            replace(IS, IS->right);
            IS->right = delNode->right;
            IS->right->parent = IS;
        }

        replace(delNode, IS);
        IS->left = delNode->left;
        IS->left->parent = IS;
        IS->color = delNode->color;
    }

    if (origColor == BLACK)
        fixDelete(child);

    delete delNode;
    cout << "Deleted key " << key << endl;
}

void inorder(Node *node)
{
    if (!node)
        return;
    inorder(node->left);
    cout << "Key: " << node->key << ", Value: " << node->val << endl;
    inorder(node->right);
}

int main()
{
    string cmd;

    while (cin >> cmd)
    {
        if (cmd == "insert")
        {
            int k, v;
            cin >> k >> v;
            insert(k, v);
        }
        else if (cmd == "find")
        {
            int k;
            cin >> k;
            find(k);
        }
        else if (cmd == "delete")
        {
            int k;
            cin >> k;
            deleteKey(k);
        }
        else if (cmd == "print")
        {
            inorder(root);
        }
        else if (cmd == "exit")
        {
            break;
        }
        else
        {
            cout << "Invalid command\n";
        }
    }
    return 0;
}
