# OutTree
```cpp
{
        /* Overview */
    /*
        OutTree is an iterative, forest-like container designed for hierarchical data management
        without a common root. It provides top-level element access via begin() and end() methods
        through traversal wrappers, supporting flat (sibling) and pre-order (depth-first) traversal modes.

        The choice of iterator traversal policy significantly impacts the scope of operations.
        Utilizing a flat iterator restricts operations to sibling nodes within the current subtree. (supports deep operations)
        Conversely, a preorder iterator facilitates traversal and operation across the entire subtree. (supports shallow operations only)
    */

        /* Key Features */
    /*
        - Forest Structure: Supports multiple independent top-level nodes, eliminating the need for a common root.
        - Traversal Modes:
            - Flat Traversal: Iterates exclusively over sibling nodes at the current level.
            - Pre-Order Traversal: Implements a depth-first iteration spanning the entire structure.
        - Iterator-Based Design: Provides control over data access and manipulation through a robust iterator interface.
        - Copy Operations: Facilitates both shallow and deep copying of individual nodes and complete subtrees.
        - Join/Unjoin Operations: Enables the programmatic merging and splitting of tree structures.
        - Memory Efficiency: Utilizes a compact internal representation, optimized for x64 architecture.
    */

        /* Memory Usage */
    /*
        - Container Size: sizeof(OutTree<int>) == 8 bytes (x64).
        - Node Size:
          - Debug: sizeof(node_type) == 80 bytes.
          - Release: sizeof(node_type) == 72 bytes.
        - View Wrapper Size: sizeof(as_flat()) == 8 bytes.
        - Iterator Size: sizeof(const_preorder_iterator) == 16 bytes (two pointers).
    */

        /* Complexity Analysis */
    /*
        - Structure - Changing Operations: O(N) worst - case, where N is the total number of nodes.
        - Get Operations (e.g., size(), begin()): O(1).
        - Pre - Order Traversal: O(N).
        - Flat Traversal: O(1) per operation.
    
        *Worst - case occurs when tree height is proportional to the number of nodes
          (e.g., a degenerate tree resembling a linked list).*
    */




    //=== Usage Examples ===//

    using OTi = OutTree<int>;
    using OTs = OutTree<std::string>;

    // Basic Construction
    OTi A_tree{
        OTi{1, {
            OTi{11, {
                OTi{111} }},
            OTi{12} }},
        OTi{2} };
    std::cout << "A_tree:\n" << A_tree << "\n";
    /*
        A_tree:
        [0] 1
        |------ [1] 11
                |------ [2] 111
        |------ [1] 12
        [0] 2
        Size: 5
    */

    OTi B_tree{
        OTi{-1, {
            OTi{-2} }} };
    std::cout << "B_tree:\n" << B_tree << "\n";
    /*
        B_tree:
        [0] -1
        |------ [1] -2
        Size: 2
    */

    OTs C_tree;
    C_tree.append(
        OTs{ "1" }.append(
            OTs{ "2" }),
        OTs{ "3" },
        OTs{ "4" }
    );
    std::cout << "C_tree:\n" << C_tree << "\n";
    /*
        C_tree:
        [0] 1
        |------ [1] 2
        [0] 3
        [0] 4
        Size: 4
    */
    
    OTs D_tree;
    D_tree.append(
        OTs{ "1" },
        OTs{ "2" }.append(
            OTs{ "3" }),
        OTs{ "4" }
    );
    std::cout << "D_tree:\n" << D_tree << "\n";
    /*
        D_tree:
        [0] 1
        [0] 2
        |------ [1] 3
        [0] 4
        Size: 4
    */



    {
        std::cout << "--- Flat Traversal ---\n";
        auto flat_view = A_tree.as_flat();

        std::cout << "Nodes of A_tree (children of root): ";
        for (auto& it : flat_view) {
            std::cout << it << " ";
        } std::cout << "\n\n";
        /*
            Nodes of A_tree (children of root): 1 2
        */
    }

    {
        std::cout << "--- Pre-order Traversal ---\n";
        auto preorder_view = A_tree.as_preorder();

        std::cout << "Nodes of A_tree (entire subtree): ";
        for (auto& it : preorder_view) {
            std::cout << it << " ";
        } std::cout << "\n\n";
        /*
            Nodes of A_tree (entire subtree): 1 11 111 12 2
        */
    }

    {
        std::cout << "--- Tree Appending ---\n";
        auto dest_ = A_tree;
        std::cout << "Initial Destination Tree (A_tree):\n";
        std::cout << dest_ << "\n";
        /*
            Initial Destination Tree (A_tree):
            [0] 1
            |------ [1] 11
                    |------ [2] 111
            |------ [1] 12
            [0] 2
            Size: 5
        */

        auto src_ = B_tree;
        std::cout << "Source Tree to be Appended (B_tree):\n";
        std::cout << src_ << "\n";
        /*
            Source Tree to be Appended (B_tree):
            [0] -1
            |------ [1] -2
            Size: 2
        */

        std::cout << "Performing append: Appending src_ to the last node of dest_ (in pre-order traversal).\n";
        dest_.append(src_);

        std::cout << "Resulting Destination Tree After Append:\n";
        std::cout << dest_ << "\n";
        /*
            Resulting Destination Tree After Append:
            [0] 1
            |------ [1] 11
                    |------ [2] 111
            |------ [1] 12
            [0] 2
            |------ [1] -1
                    |------ [2] -2
            Size: 7
        */
    }

    {
        std::cout << "--- Shallow Comparison ---\n";
        auto cmp1 = C_tree;
        auto pv_cmp1 = cmp1.pre();
        std::cout << "Tree 'cmp1' (C_tree):\n";
        std::cout << cmp1 << "\n";
        /*
            Tree 'cmp1' (C_tree):
            [0] 1
            |------ [1] 2
            [0] 3
            [0] 4
            Size: 4
        */

        auto cmp2 = D_tree;
        auto pv_cmp2 = cmp2.pre();
        std::cout << "Tree 'cmp2' (D_tree):\n";
        std::cout << cmp2 << "\n";
        /*
            Tree 'cmp2' (D_tree):
            [0] 1
            [0] 2
            |------ [1] 3
            [0] 4
            Size: 4
        */

        std::cout << "Comparing pre-order traversals (ranges) of cmp1 and cmp2.\n";
        std::cout << "Expected behavior: Compares two ranges of nodes as sequences (values only).\n";
        std::cout << "Note: Begin/end iterators should belong to the same subtree (obtained from one parent node or container).\n";

        std::cout << "Result: " <<
            (cmp1.compare(pv_cmp1.cbegin(), pv_cmp1.cend(), pv_cmp2.begin(), pv_cmp2.end())
                ? "range_cmp1 == range_cmp2"
                : "range_cmp1 != range_cmp2"
                ) << "\n\n";
        /*
            Result: range_cmp1 == range_cmp2
        */
    }

    {
        std::cout << "--- Deep Comparison ---\n";
        auto cmp1 = C_tree;
        auto fv_cmp1 = cmp1.flat();
        std::cout << "Tree 'cmp1' (C_tree):\n";
        std::cout << cmp1 << "\n";
        /*
            Tree 'cmp1' (C_tree):
            [0] 1
            |------ [1] 2
            [0] 3
            [0] 4
            Size: 4
        */

        auto cmp2 = C_tree;
        // Change '2' to '-2'
        *++cmp2.pre().begin() = "-2";
        auto fv_cmp2 = cmp2.flat();
        std::cout << "Tree 'cmp2' (modified C_tree):\n";
        std::cout << cmp2 << "\n";
        /*
            Tree 'cmp2' (modified C_tree):
            [0] 1
            |------ [1] -2
            [0] 3
            [0] 4
            Size: 4
        */

        std::cout << "Comparing full flat-order traversals (ranges) of cmp1 and cmp2.\n";
        std::cout << "Expected behavior: Compares two ranges of nodes (and its respective subtrees).\n";
        std::cout << "Note: Begin/end iterators should belong to the same subtree (obtained from one parent node or container).\n";

        std::cout << "Result: " <<
            (cmp1.deep_compare(fv_cmp1.cbegin(), fv_cmp1.cend(), fv_cmp2.begin(), fv_cmp2.end())
                ? "range_cmp1 == range_cmp2"
                : "range_cmp1 != range_cmp2"
                ) << "\n\n";
        /*
            Result: range_cmp1 != range_cmp2
        */
    }

    {
        std::cout << "--- Shallow Copying ---\n";
        auto dest_ = A_tree;
        auto pv_dest_ = dest_.pre();
        std::cout << "Initial Destination Tree ('dest_', copy of A_tree):\n";
        std::cout << dest_ << "\n";
        /*
            Initial Destination Tree ('dest_', copy of A_tree):
            [0] 1
            |------ [1] 11
                    |------ [2] 111
            |------ [1] 12
            [0] 2
            Size: 5
        */

        auto src_ = B_tree;
        auto pv_src_ = src_.pre();
        std::cout << "Initial Source Tree ('src_', copy of B_tree):\n";
        std::cout << src_ << "\n";
        /*
            Initial Source Tree ('src_', copy of B_tree):
            [0] -1
            |------ [1] -2
            Size: 2
        */

        std::cout << "Copying nodes from src_ (pre-order) before the first node of dest_ (pre-order traversal).\n";
        std::cout << "Operation: dest_.copy(dest_.pre().cend(), src_.pre().begin(), src_.pre().end());\n";
        std::cout << "Expected behavior: Appends a shallow copy of src_'s entire pre-order range before 'where' position.\n";

        dest_.copy(pv_dest_.cend(), pv_src_.begin(), pv_src_.end());
        std::cout << "Resulting Destination Tree After First Copy:\n";
        std::cout << dest_ << "\n";
        /*
            Resulting Destination Tree After First Copy:
            [0] 1
            |------ [1] 11
                    |------ [2] 111
            |------ [1] 12
            [0] 2
            [0] -1
            [0] -2
            Size: 7
        */
    }

    {
        std::cout << "--- Deep Copying ---\n";
        auto dest_ = A_tree;
        auto fv_dest_ = dest_.flat();
        std::cout << "Initial Destination Tree ('dest_', copy of A_tree):\n";
        std::cout << dest_ << "\n";
        /*
            Initial Destination Tree ('dest_', copy of A_tree):
            [0] 1
            |------ [1] 11
                    |------ [2] 111
            |------ [1] 12
            [0] 2
            Size: 5
        */

        auto src_ = B_tree;
        auto fv_src_ = src_.flat();
        std::cout << "Initial Source Tree ('src_', copy of B_tree):\n";
        std::cout << src_ << "\n";
        /*
            Initial Source Tree ('src_', copy of B_tree):
            [0] -1
            |------ [1] -2
            Size: 2
        */

        std::cout << "Copying nodes from src_ (flat view) to the end of dest_ (flat view).\n";
        std::cout << "Operation: dest_.deepcopy(dest_.flat().cend(), src_.flat().begin(), src_.flat().end());\n";
        std::cout << "Expected behavior: Appends a deep copy of the range from src_'s flat view.\n";

        dest_.deep_copy(fv_dest_.cend(), fv_src_.begin(), fv_src_.end());
        std::cout << "Resulting Destination Tree After second Copy:\n";
        std::cout << dest_ << "\n";
        /*
            Resulting Destination Tree After second Copy:
            [0] 1
            |------ [1] 11
                    |------ [2] 111
            |------ [1] 12
            [0] 2
            [0] -1
            |------ [1] -2
            Size: 7
        */
    }

    {
        std::cout << "--- Element Construction and Insertion ---\n";
        OTs sTree;
        std::cout << "Initial tree 'sTree':\n";
        std::cout << sTree << "\n";
        /*
            Initial tree 'sTree':
            <empty>
            Size: 0
        */

        std::cout << "Performing emplace: Constructing an element in place at the end of the flat view.\n";
        std::cout << "Operation: sTree.emplace(sTree.flat().end(), 5, 'X');\n";
        std::cout << "Expected behavior: A new node with content 'XXXXX' is created directly within the tree.\n";
        sTree.emplace(sTree.flat().end(), 5, 'X');
        std::cout << "'sTree' after emplace:\n";
        std::cout << sTree << "\n";
        /*
            'sTree' after emplace:
            [0] XXXXX
            Size: 1
        */

        std::cout << "Performing insert 1: Inserting a single string value 'a' by copy at the end of the flat view.\n";
        std::cout << "Operation: sTree.insert(sTree.flat().end(), \"a\");\n";

        sTree.insert(sTree.flat().end(), "a");

        std::cout << "Performing insert 2: Inserting a range of string values {\"b\", \"c\", \"d\"} from an initializer list at the end of the flat view.\n";
        std::cout << "Operation: sTree.insert(sTree.flat().end(), { \"b\", \"c\", \"d\" });\n";

        sTree.insert(sTree.flat().end(), { "b", "c", "d" });

        std::cout << "Final tree 'sTree' after all inserts:\n";
        std::cout << sTree << "\n";
        /*
            Final tree 'sTree' after all inserts:
            [0] XXXXX
            [0] a
            [0] b
            [0] c
            [0] d
            Size: 5
        */
    }

    {
        std::cout << "--- Tree Joining Operation ---\n";
        auto dest_ = A_tree;
        std::cout << "Initial Destination Tree ('dest_', copy of A_tree):\n";
        std::cout << dest_ << "\n";
        /*
            Initial Destination Tree ('dest_', copy of A_tree):
            [0] 1
            |------ [1] 11
                    |------ [2] 111
            |------ [1] 12
            [0] 2
            Size: 5
        */

        auto src_ = B_tree;
        std::cout << "Initial Source Tree ('src_', copy of B_tree):\n";
        std::cout << src_ << "\n";
        /*
            Initial Source Tree ('src_', copy of B_tree):
            [0] -1
            |------ [1] -2
            Size: 2
        */

        std::cout << "Performing join: Moving contents of src_ into dest_ at the end of dest_'s flat view.\n";
        std::cout << "Operation: dest_.join(dest_.flat().cend(), src_);\n";
        std::cout << "Expected behavior: The nodes from 'src_' are moved (not copied) into 'dest_' at the specified position. 'src_' will likely become empty or in a moved-from state after the operation.\n";
        
        dest_.join(dest_.flat().cend(), src_);
        std::cout << "Resulting Destination Tree After Join:\n";
        std::cout << dest_ << "\n";
        /*
            Resulting Destination Tree After Join:
            [0] 1
            |------ [1] 11
                    |------ [2] 111
            |------ [1] 12
            [0] 2
            [0] -1
            |------ [1] -2
            Size: 7
        */
    }

    {
        std::cout << "--- Tree Unjoining Operation ---\n";
        auto src_ = A_tree;
        std::cout << "Initial Source Tree ('src_', copy of A_tree):\n";
        std::cout << src_ << "\n";
        /*
            Initial Source Tree ('src_', copy of A_tree):
            [0] 1
            |------ [1] 11
                    |------ [2] 111
            |------ [1] 12
            [0] 2
            Size: 5
        */
        std::cout << "Locating node with value 11 in src_ (pre-order traversal)...\n";
        auto it_to_11 = std::find(src_.pre().begin(), src_.pre().end(), 11);
        assert(it_to_11 != src_.pre().end());

        decltype(src_) unjoined_ = src_.unjoin(it_to_11);
        std::cout << "Source Tree ('src_') After Unjoin:\n";
        std::cout << src_ << "\n";
        /*
            Source Tree ('src_') After Unjoin:
            [0] 1
            |------ [1] 12
            [0] 2
            Size: 3
        */
        std::cout << "Unjoined Tree ('unjoined_') After Unjoin:\n";
        std::cout << unjoined_ << "\n";
        /*
            Unjoined Tree ('unjoined_') After Unjoin:
            [0] 11
            |------ [1] 111
            Size: 2
        */
    }

    {
        std::cout << "--- Tree Moving Operations ---\n";
        auto src_ = A_tree;
        auto dest_ = decltype(src_){};
        std::cout << "Initial Source Tree ('src_', copy of A_tree):\n";
        std::cout << src_ << "\n";
        /*
            Initial Source Tree ('src_', copy of A_tree):
            [0] 1
            |------ [1] 11
                    |------ [2] 111
            |------ [1] 12
            [0] 2
            Size: 5
        */

        std::cout << "Locating node with value 11 in src_ (pre-order traversal)...\n";
        auto it_to_11 = std::find(src_.pre().begin(), src_.pre().end(), 11);
        assert(it_to_11 != src_.pre().end());

        std::cout << "Node 11 found. Performing move operation within 'src_'.\n";
        std::cout << "Operation: src_.move(src_.pre().begin(), it_to_11);\n";
        std::cout << "Expected behavior: The node at 'it_to_11' is moved before the element at 'where' position.\n";
        
        src_.move(src_.flat().begin(), decltype(src_)::flat_iterator(it_to_11));
        std::cout << "Source Tree ('src_') After First Move (node 11 moved to front):\n";
        std::cout << src_ << "\n";
        /*
            Source Tree ('src_') After First Move (node 11 moved to front):
            [0] 11
            |------ [1] 111
            [0] 1
            |------ [1] 12
            [0] 2
            Size: 5
        */
    }
```
