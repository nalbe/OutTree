#pragma once
#include <utility>
#include <iterator>
#include <stdexcept>
#include <ostream>



namespace nsOutTree
{

	template < typename, bool, typename> class Iterator;
	template <typename, typename> class Container;



	template <typename TValue, typename TSize = std::size_t, typename TDiff = std::ptrdiff_t>
	struct BasicTraits
	{
		// Core types passed as template parameters
		using value_type       = TValue;
		using size_type        = TSize;
		using difference_type  = TDiff;

		// Derived standard aliases (convenience)
		using pointer          = value_type*;
		using const_pointer    = const value_type*;
		using reference        = value_type&;
		using const_reference  = const value_type&;
	};



	//=== Manages node-specific operations and properties for the container's structure ===//
	template < typename TContainer >
	class NodeManager
	{
	public:
		// Type aliases propagated from the traits
		using value_type       = typename TContainer::value_type;
		using pointer          = typename TContainer::pointer;
		using const_pointer    = typename TContainer::const_pointer;
		using reference        = typename TContainer::reference;
		using const_reference  = typename TContainer::const_reference;
		using size_type        = typename TContainer::size_type;
		using difference_type  = typename TContainer::difference_type;


	private:
		// Helper trait to detect if operator<< is available for a given type T
		template <typename T, typename = void> struct has_ostream_operator : std::false_type {};
		template <typename T> struct has_ostream_operator<
			T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<const T&>())>
		> : std::true_type {};


	private:
		//=== Base node linkage class using CRTP ===//
		template < typename TDerived >
		class NodeBase
		{
		public:
			// Type aliases
			using self_type  = typename NodeBase;
			using node_type  = typename TDerived;

		private:
			// Friend declarations
			friend class NodeManager;

		private:
			// Permanent pointer to the node itself
			node_type* pSelf{ static_cast<node_type*>(this) };

			// Pointer to parent’s pSelf: 
			//   nullptr       = root sentinel
			//   &pSelf        = unlinked node
			node_type** pParent{};

			// Pointer to previous sibling’s pSelf: 
			//   parent->pREnd = first child
			//   nullptr       = unlinked
			node_type** pPrevSibling{};

			// Pointer to next sibling’s pSelf: 
			//   parent->pEnd  = last child
			//   nullptr       = unlinked
			node_type** pNextSibling{};

			// Reverse-end sentinel for child list:
			//   pSelf         = no children
			//   begin         = first children
			node_type* pREnd{ pSelf };

			// End sentinel for child list:
			//   pSelf         = no children
			//   rbegin        = last children
			node_type* pEnd{ pSelf };

			// Count of direct child nodes (immediate descendants only)
			size_type nChildCount{};

			// Total nodes in subtree (includes self and all descendants)
			size_type nSize{ 1 };

		private:
			// Deleted constructors
			NodeBase(const self_type&) = delete;
			NodeBase(self_type&&) = delete;
			// Deleted operators
			self_type& operator =(const self_type&) = delete;
			self_type& operator =(self_type&&) = delete;
			friend bool operator ==(const self_type&, const self_type&) = delete;
			friend bool operator !=(const self_type&, const self_type&) = delete;

		public:
			// Default destructor
			~NodeBase() = default;
			// Default constructor
			NodeBase() = default;
		};


	private:
		//=== Represents a concrete node type ===//
		class NodeData : public NodeBase<NodeData>
		{
		public:
			// Type aliases
			using self_type      = NodeData;
			using node_base      = NodeBase<self_type>;

		private:
			// Friend declarations
			friend class NodeManager;

		private:
			// Raw storage for the value_type object to allow placement new
			alignas(value_type) unsigned char data[sizeof(value_type)];

#ifdef _DEBUG
			value_type* pData = reinterpret_cast<value_type*>(&data);
#endif // _DEBUG

		private:
			// Deleted constructors
			NodeData() = delete;
			NodeData(const self_type&) = delete;
			NodeData(self_type&&) = delete;
			// Deleted operators
			self_type& operator =(const self_type&) = delete;
			self_type& operator =(self_type&&) = delete;
			friend bool operator ==(const self_type&, const self_type&) = delete;
			friend bool operator !=(const self_type&, const self_type&) = delete;

		public:
			// Destructor: Destroys the stored value_type object
			~NodeData()
			{
				reinterpret_cast<reference>(data).~value_type();
			}
			// Constructor taking const_reference (uses placement new)
			NodeData(const value_type& value_)
			{
				new(data) value_type(value_);
			}
			// Constructor taking rvalue_type (uses placement new with move)
			NodeData(value_type&& value_) noexcept
			{
				new(data) value_type(std::move(value_));
			}
			// Perfect forwarding constructor for emplace
			template <typename... Args>
			NodeData(Args&&... args)
			{
				new(data) value_type(std::forward<Args>(args)...);
			}
		};


	public:
		// Type aliases
		using node_type           = typename NodeData::self_type;
		using node_base           = typename node_type::node_base;
		using node_pointer        = node_type**;
		using const_node_pointer  = const node_type* const*;


	public:
		//=== Pre-order traverse policy ===//
		struct PreorderTraversePolicy_
		{
			// @brief Retrieves the previous node in a reverse preorder traversal of a tree.
			//
			// This function serves as a helper for performing a reverse preorder traversal.
			// It identifies the node that precedes the current node in this reverse sequence,
			// starting from an end sentinel and moving backward through the tree.
			//
			// @param node_ The current node or end sentinel in the pre-order traversal sequence.
			//
			// @return A pointer to the previous node in the reverse pre-order traversal.
			template <typename NodePtr_>
			static NodePtr_ policy_prev(NodePtr_ node_) noexcept
			{
				return prev_preorder_raw(node_);
			}

			// @brief Retrieves the next node in a depth-first (preorder) traversal of a tree.
			//
			// Advances the traversal by returning the node that follows the given node in a preorder sequence:
			// root first, then children from left to right.
			//
			// @param node_ The current node in the traversal.
			// @param end_  The global end sentinel marking completion of the overall traversal scope.
			// @return A pointer to the next node in preorder, or the global end sentinel.
			template <typename NodePtr_>
			static NodePtr_ policy_next(NodePtr_ node_, const_node_pointer end_) noexcept
			{
				return next_preorder_raw(node_, end_);
			}
		};

		//=== Flat traverse policy ===//
		struct FlatTraversePolicy_
		{
			// @brief Retrieves the previous sibling of a node in sibling order.
			//
			// @param node_ Node to find the previous sibling for. Must not be null.
			// @return A pointer to the previous sibling in the sibling sequence.
			//
			// @details Behavior depends on node type:
			//   - For the **end sentinel**: Returns the node itself.
			//   - For regular nodes: Returns the immediate previous sibling.
			template <typename NodePtr_>
			static NodePtr_ policy_prev(NodePtr_ node_) noexcept
			{
				return is_end(node_)
					? self(node_)
					: prev_sibling_raw(node_);
			}

			// @brief Retrieves the next sibling of a node in sibling order.
			//
			// @param node_ Node to find the next sibling for. Must not be null.
			// @return Pointer to the next sibling in the sibling sequence.
			//
			// @details Behavior depends on node type:
			//   - For the **reverse end sentinel**: Returns the node itself.
			//   - For regular nodes: Returns the immediate next sibling.
			template <typename NodePtr_>
			static NodePtr_ policy_next(NodePtr_ node_, [[maybe_unused]] const_node_pointer = nullptr) noexcept
			{
				//return is_sentinel(node_)
				return is_rend(node_)
					? self(node_)
					: next_sibling_raw(node_);
			}
		};


	private:
		// Helper print function, enabled based on operator<< existence
		template <typename T> static typename std::enable_if<has_ostream_operator<T>::value, void>::type
			print_node_value(std::ostream& os_, const T& value_)
		{
			os_ << value_;
		}
		// Helper print function for types without operator<<
		template <typename T> static typename std::enable_if<!has_ostream_operator<T>::value, void>::type
			print_node_value(std::ostream& os_, const T&)
		{
			os_ << "<unprintable>"; // Placeholder
		}


	private:
		// Helper function to access the previous sibling const pointer
		static const_node_pointer prev_sibling_raw(const_node_pointer node_)
		{
			return (**node_).pPrevSibling;
		}

		// Helper function to access the previous sibling pointer
		static node_pointer prev_sibling_raw(node_pointer node_)
		{
			return const_cast<node_pointer>(
				prev_sibling_raw(
					static_cast<const_node_pointer>(node_)
				));
		}

		// Helper function to access the next sibling const pointer
		static const_node_pointer next_sibling_raw(const_node_pointer node_)
		{
			return (**node_).pNextSibling;
		}

		// Helper function to access the next sibling pointer
		static node_pointer next_sibling_raw(node_pointer node_)
		{
			return const_cast<node_pointer>(
				next_sibling_raw(
					static_cast<const_node_pointer>(node_)
				));
		}


		// Helper function to access the previous sibling const pointer
		static const_node_pointer prev_preorder_raw(const_node_pointer node_)
		{
			// Helper to find the deepest rightmost descendant (or the node itself if no children)
			// The time complexity is O(N) in the worst-case scenario,
			// where the height proportional to the total number of nodes N in the container
			auto find_deepest_rightmost_descendant = [](const_node_pointer pNode_) {
				while (has_children(pNode_)) { pNode_ = get_rbegin(pNode_); }  // Move to the last child if present
				return pNode_;  // If no children, return
				};

			// If current node is the end sentinel, go to the last child of its parent
			if (is_end(node_)) {
				return find_deepest_rightmost_descendant(self_from_end(node_));
			}
			// Move to the previous sibling if present
			if (!is_sentinel(prev_sibling_raw(node_))) {
				return find_deepest_rightmost_descendant(prev_sibling_raw(node_));
			}
			// If not end sentinel and no previous sibling, go up to the parent's previous sibling
			return get_parent(node_);
		}

		// Helper function to access the previous sibling pointer
		static node_pointer prev_preorder_raw(node_pointer node_)
		{
			return const_cast<node_pointer>(
				prev_preorder_raw(
					static_cast<const_node_pointer>(node_)
				));
		}

		// Helper function to access the next sibling const pointer
		static const_node_pointer next_preorder_raw(const_node_pointer node_, const_node_pointer end_)
		{
			// Descend into children if available
			if (has_children(node_)) { return get_begin(node_); }

			// Loop until we find the next node or reach the end
			while (get_end(node_) != end_) {
				// Move to the next sibling if present
				if (!is_sentinel(next_sibling_raw(node_))) { return next_sibling_raw(node_); }
				// If no children and no next sibling, go up to the parent's next sibling
				node_ = get_parent(node_);
			}
			// If reached the end of sub-tree, return end sentinel
			return get_end(node_);
		}

		// Helper function to access the next sibling pointer
		static node_pointer next_preorder_raw(node_pointer node_, const_node_pointer end_)
		{
			return const_cast<node_pointer>(
				next_preorder_raw(
					static_cast<const_node_pointer>(node_), end_
				));
		}


	private:
		// Helper function to access the self const pointer
		static const node_type*& self_raw(const_node_pointer node_)
		{
			return (**node_).pSelf;
		}

		// Helper function to access the self pointer
		static node_type*& self_raw(node_pointer node_)
		{
			return (**node_).pSelf;
		}

		// Helper function to direct access the first child const pointer
		static const_node_pointer begin_raw(const_node_pointer node_)
		{
			return &(**node_).pREnd->pSelf;
		}

		// Helper function to direct access the first child pointer
		static node_pointer begin_raw(node_pointer node_)
		{
			return const_cast<node_pointer>(
				begin_raw(
					static_cast<const_node_pointer>(node_)
				));
		}

		// Helper function to direct access the last child const pointer
		static const_node_pointer rbegin_raw(const_node_pointer node_)
		{
			return &(**node_).pEnd->pSelf;
		}

		// Helper function to direct access the last child pointer
		static node_pointer rbegin_raw(node_pointer node_)
		{
			return const_cast<node_pointer>(
				rbegin_raw(
					static_cast<const_node_pointer>(node_)
				));
		}


	private:
		// Increases the total subtree count upwards to the root
		// The time complexity is O(N), where N = depth of inserted node
		static void increase_sizes_upwards(node_pointer node_, size_type value_)
		{
			// Traverse up the parent chain until it is valid node
			for (node_pointer it_{ get_parent(node_) }; is_valid(it_); it_ = get_parent(it_)) {
				// Add the current node's subtree count to parents
				(**it_).nSize += value_;
			}
		}

		// Decreases the total subtree count upwards to the root
		// The time complexity is O(N), where N = depth of removed node
		static void decrease_sizes_upwards(node_pointer node_, size_type value_)
		{
			// Traverse up the parent chain
			for (node_pointer it_{ get_parent(node_) }; is_valid(it_); it_ = get_parent(it_)) {
				// Subtract the current node's subtree count from parents
				(**it_).nSize -= value_;
			}
		}

		// Helper function to link a node to new parent's sibling list
		static node_pointer link_impl(node_pointer where_, node_pointer node_)
		{
			// Inserting into an empty sub-list
			if (is_end_sentinel_of_empty_sublist(where_)) {
				(**node_).pParent = self(where_);  // parent pointer
				(**node_).pPrevSibling = get_rend(get_parent(node_));  // no previous sibling
				(**node_).pNextSibling = get_end(get_parent(node_));  // no next sibling

				(**get_parent(node_)).pREnd = self_raw(node_);  // If no siblings, this is the new first child
				(**get_parent(node_)).pEnd = self_raw(node_);  // If no next sibling, this is the new last child
			}
			// Inserting at the begin of a list
			else if (is_begin(where_)) {
				(**node_).pParent = get_parent(where_);  // parent pointer
				(**node_).pPrevSibling = get_rend(get_parent(node_));  // no previous sibling
				(**node_).pNextSibling = self(where_);  // next sibling

				(**get_parent(node_)).pREnd = self_raw(node_);  // If no siblings, this is the new first child
				(**next_sibling_raw(node_)).pPrevSibling = self(node_);  // Update next sibling's previous pointer	
			}
			// Inserting at the end of a list using sentinel
			else if (is_end(where_)) {
				(**node_).pParent = get_parent(where_);  // parent pointer
				(**node_).pPrevSibling = self(where_);  // previous sibling
				(**node_).pNextSibling = get_end(get_parent(node_));  // no next sibling

				(**prev_sibling_raw(node_)).pNextSibling = self(node_);  // Update previous sibling's next pointer
				(**get_parent(node_)).pEnd = self_raw(node_);  // If no next sibling, this is the new last child
			}
			// Inserting before an existing node in a sibling list
			else {
				(**node_).pParent = get_parent(where_);  // parent pointer
				(**node_).pPrevSibling = prev_sibling_raw(where_);  // previous sibling
				(**node_).pNextSibling = self(where_);  // next sibling

				(**prev_sibling_raw(node_)).pNextSibling = self(node_);  // Update previous sibling's next pointer
				(**next_sibling_raw(node_)).pPrevSibling = self(node_);  // Update next sibling's previous pointer	

			}
			// Increment parent's child count
			++(**get_parent(node_)).nChildCount;
			return node_;
		}

		// Helper function to unlink node from its parent's sibling list
		static node_pointer unlink_impl(node_pointer node_)
		{
			--(**get_parent(node_)).nChildCount;  // Decrement parent's child count

			if (!is_sentinel(prev_sibling_raw(node_))) {
				// Link previous sibling to the next sibling
				(**prev_sibling_raw(node_)).pNextSibling = next_sibling_raw(node_);
			}
			else {  // Link parent's begin pointer
				(**get_parent(node_)).pREnd = is_sentinel(next_sibling_raw(node_))
					? self_raw(get_parent(node_))
					: self_raw(next_sibling_raw(node_));
			}

			if (!is_sentinel(next_sibling_raw(node_))) {
				// Link next sibling to the previous sibling
				(**next_sibling_raw(node_)).pPrevSibling = prev_sibling_raw(node_);
			}
			else {  // Link parent's end pointer
				(**get_parent(node_)).pEnd = is_sentinel(prev_sibling_raw(node_))
					? self_raw(get_parent(node_))
					: self_raw(prev_sibling_raw(node_));
			}

			// Reset pointers for the removed node
			(**node_).pParent = (**node_).pPrevSibling = (**node_).pNextSibling = self(node_);
			return node_;
		}

		// Helper function to move node before the position indicated by where_
		static node_pointer move_impl(node_pointer where_, node_pointer node_)
		{
			unlink_impl(node_);
			link_impl(where_, node_);
			return node_;
		}

		// Helper function to copy node before the position indicated by where_
		static node_pointer shallow_copy_impl(const_node_pointer node_)
		{
			node_pointer copied_ = self(new node_type(data_ref(node_)));
			return copied_;
		}

		// Helper function to copy node before the position indicated by where_
		static node_pointer deep_copy_impl(const_node_pointer node_)
		{
			node_pointer copied_ = self(new node_type(data_ref(node_)));
			if (has_children(node_)) {
				if (!for_each<PreorderTraversePolicy_>(
					copied_, get_end(copied_),
					node_, get_end(node_),
					[](node_pointer lhs_, const_node_pointer rhs_) {
						// Iterate through children of rhs_ and copy them to lhs_
						for (auto it_{ get_begin(rhs_) }; it_ != get_end(rhs_); it_ = next_sibling_raw(it_)) {
							// Create a new node passing value as param
							node_pointer child_ = self(new node_type(data_ref(it_)));
							// Insert as the last child of lhs_
							link_impl(get_end(lhs_), child_);
						}
						// Copy the total count
						(**lhs_).nSize = get_size(rhs_);
						return true;  // Continue iteration
					}
				)) { return nullptr; }
			}
			return copied_;
		}

		// Helper function to compare data
		template <typename BinPred_>
		static bool shallow_compare_impl(const_node_pointer first_, const_node_pointer second_, BinPred_&& equal_)
		{
			return equal_(data_ref(first_), data_ref(second_));
		}

		// Helper function to compare data and counters
		template <typename BinPred_>
		static bool deep_compare_impl(const_node_pointer first_, const_node_pointer second_, BinPred_&& equal_)
		{
			return for_each<PreorderTraversePolicy_>(
				first_, get_end(first_),
				second_, get_end(second_),
				[&equal_](auto first_, auto second_) {
					return (get_size(first_) == get_size(second_))
						and (get_child_count(first_) == get_child_count(second_))
						and equal_(data_ref(first_), data_ref(second_));
				}
			);
		}


	public:
		// Interface function to link a node
		static node_pointer link(node_pointer where_, node_pointer node_)
		{
			if (where_ != node_) {
				link_impl(where_, node_);
				increase_sizes_upwards(node_, get_size(node_));
			}
			return node_;
		}

		// Interface function to unlink node
		static node_pointer unlink(node_pointer node_)
		{
			decrease_sizes_upwards(node_, get_size(node_));
			unlink_impl(node_);
			return node_;
		}

		// Interface function to remove node
		static node_pointer remove(node_pointer node_)
		{
			auto following_ = next_sibling_raw(node_);
			unlink(node_);
			// Traverse sub-tree in reverse order and delete them
			for_each_reverse<PreorderTraversePolicy_>(
				get_end(node_), node_,
				[](node_pointer node_) {
					delete* node_;
					return true;
				}
			);
			return following_;
		}

		// Interface function to remove node in range [begin,end) if predicate
		template <typename TTraversePolicy, typename UnPred_>
		static size_type remove_if(node_pointer begin_, node_pointer end_, UnPred_&& pred_)
		{
			size_type removedCnt_{};
			// Removes all elements for which predicate returns true
			for_each_reverse<TTraversePolicy>(
				end_, begin_,
				[&removedCnt_, &pred_](node_pointer node_) {
					if (pred_(data_ref(const_node_pointer(node_)))) {
						removedCnt_ += get_size(node_);
						remove(node_);
					}
					return true;  // Continue
				}
			);
			return removedCnt_;
		}

		// Interface function to shallow copy node before the position indicated by where_ (exclude sub-tree)
		static node_pointer shallow_copy(node_pointer where_, const_node_pointer node_)
		{
			auto copied_ = shallow_copy_impl(node_);
			link_impl(where_, copied_);
			increase_sizes_upwards(copied_, 1u);
			return copied_;
		}

		// Interface function to shallow copy range of nodes [begin, end) before the position indicated by where_
		template <typename TTraversePolicy>
		static node_pointer shallow_copy(node_pointer where_, const_node_pointer begin_, const_node_pointer end_)
		{
			if (is_empty_range(begin_, end_)) { return where_; }
			size_type copiedSizeCnt_{};
			for_each_reverse<TTraversePolicy>(
				end_, begin_,
				[&where_, &copiedSizeCnt_](auto node_) {
					auto copied_ = shallow_copy_impl(node_);
					link_impl(where_, copied_);
					++copiedSizeCnt_;
					where_ = copied_;
					return true;
				}
			);
			increase_sizes_upwards(where_, copiedSizeCnt_);
			return where_;
		}

		// Interface function to deep copy node before the position indicated by where_ (exclude sub-tree)
		static node_pointer deep_copy(node_pointer where_, const_node_pointer node_)
		{
			auto copied_ = deep_copy_impl(node_);
			link_impl(where_, copied_);
			increase_sizes_upwards(copied_, get_size(copied_));
			return copied_;
		}

		// Interface function to copy range of nodes [begin, end) before the position indicated by where_
		template <typename TTraversePolicy>
		static node_pointer deep_copy(node_pointer where_, const_node_pointer begin_, const_node_pointer end_)
		{
			size_type copiedSizeCnt_{};
			for_each_reverse<TTraversePolicy>(
				end_, begin_,
				[&where_, &copiedSizeCnt_](auto node_) {
					auto copied_ = deep_copy_impl(node_);
					link_impl(where_, copied_);
					copiedSizeCnt_ += get_size(node_);
					where_ = copied_;
					return true;
				}
			);
			increase_sizes_upwards(where_, copiedSizeCnt_);
			return where_;
		}

		// Interface function to move node before the position indicated by where_
		static node_pointer move(node_pointer where_, node_pointer node_)
		{
			decrease_sizes_upwards(node_, get_size(node_));
			move_impl(where_, node_);
			increase_sizes_upwards(node_, get_size(node_));
			return node_;
		}

		// Interface function to move range of nodes [begin, end) before the position indicated by where_
		template <typename TTraversePolicy>
		static node_pointer move(node_pointer where_, node_pointer begin_, node_pointer end_)
		{
			size_type movedSizeCnt_{};
			for_each_reverse<TTraversePolicy>(
				end_, begin_,
				[&where_, &movedSizeCnt_](auto node_) {
					decrease_sizes_upwards(node_, get_size(node_));
					where_ = move_impl(where_, node_);
					movedSizeCnt_ += get_size(node_);
					return true;
				}
			);
			increase_sizes_upwards(where_, movedSizeCnt_);
			return where_;
		}

		// Interface function to shallow compare two nodes
		template <typename TTraversePolicy, typename BinPred_>
		static bool shallow_compare(const_node_pointer first_, const_node_pointer second_, BinPred_&& equal_)
		{
			return shallow_compare_impl(first_, second_, std::forward<BinPred_>(equal_));
		}

		// Interface function to shallow compare range of nodes
		template <typename TTraversePolicy, typename BinPred_>
		static bool shallow_compare(const_node_pointer lbegin_, const_node_pointer lend_,
			const_node_pointer rbegin_, const_node_pointer rend_, BinPred_&& equal_)
		{
			return for_each<TTraversePolicy>(
				lbegin_, lend_,
				rbegin_, rend_,
				[&equal_](auto first_, auto second_) {
					return shallow_compare_impl(first_, second_, std::forward<BinPred_>(equal_));
				}
			);
		}

		// Interface function to deep compare two nodes
		template <typename TTraversePolicy, typename BinPred_>
		static bool deep_compare(const_node_pointer first_, const_node_pointer second_, BinPred_&& equal_)
		{
			return deep_compare_impl(first_, second_, std::forward<BinPred_>(equal_));
		}

		// Interface function to deep compare range of nodes
		template <typename TTraversePolicy, typename BinPred_>
		static bool deep_compare(const_node_pointer lbegin_, const_node_pointer lend_,
			const_node_pointer rbegin_, const_node_pointer rend_, BinPred_&& equal_)
		{
			return for_each<TTraversePolicy>(
				lbegin_, lend_,
				rbegin_, rend_,
				[&equal_](auto first_, auto second_) {
					return deep_compare_impl(first_, second_, std::forward<BinPred_>(equal_));
				}
			);
		}

		// Interface function to swap_nodes pair of nodes
		static void swap_nodes(node_pointer first_, node_pointer second_)
		{
			if (first_ == second_) { return; }
			// Special case: Adjacent nodes (A <-> B)
			if (next_sibling_raw(first_) == second_) {
				unlink(first_);
				link(next_sibling_raw(second_), first_);
				return;
			}
			// Handle reverse adjacency
			if (next_sibling_raw(second_) == first_) {
				swap_nodes(second_, first_);
				return;
			}
			// General case: Non-adjacent nodes
			auto first_pos_ = next_sibling_raw(first_);
			unlink(first_);
			link(next_sibling_raw(second_), first_);
			unlink(second_);
			link(first_pos_, second_);
		}

		// Interface function to formatted output
		static std::ostream& formatted_stream(std::ostream& os_, const_node_pointer node_)
		{
			if (!has_children(node_)) {
				os_ << "<empty>\n";
				return os_;
			}
			size_type depth_{};

			for (auto it_{ get_begin(node_) }, end_{ get_end(node_) }; it_ != end_; ) {
				// Print indentation and node
				if (depth_ > 0) {
					for (size_type i{}; i + 1 < depth_; ++i) { os_ << "        "; }
					os_ << "|------ ";
				}
#ifdef _DEBUG
				os_ << "[" << depth_ << "] ";
#endif // _DEBUG
				print_node_value(os_, data_ref(it_));
				os_ << "\n";

				// Move to the next node in preorder and adjust depth
				if (has_children(it_)) {
					it_ = get_begin(it_);  // Go to the first child
					++depth_;
				}
				else {
					// Loop to ascend until a sibling is found or we reach the end of the entire tree
					while (get_end(it_) != end_ and is_sentinel(next_sibling_raw(it_))) {
						it_ = get_parent(it_);
						--depth_;
					}
					if (get_end(it_) != end_) { it_ = next_sibling_raw(it_); }
					else { it_ = get_end(it_); }
				}
			}
#ifdef _DEBUG
			os_ << "Size: " << get_size(node_) << "\n";
#endif // _DEBUG
			return os_;
		}


	public:
		// Helper function to access the parent const pointer
		static const_node_pointer get_parent(const_node_pointer node_)
		{
			return (**node_).pParent;
		}

		// Helper function to access the parent pointer
		static node_pointer get_parent(node_pointer node_)
		{
			return (**node_).pParent;
		}

		// Helper function to retrieve the 'self' const pointer referenced by the 'const node_base*'
		static const_node_pointer self(const node_base* node_)
		{
			return &node_->pSelf;
		}

		// Helper function to retrieve the 'self' pointer referenced by the 'node_base*'
		static node_pointer self(node_base* node_)
		{
			return &node_->pSelf;
		}

		// Helper function to retrieve the 'self' const pointer referenced by the 'const_node_pointer'
		static const_node_pointer self(const_node_pointer node_)
		{
			return &(**node_).pSelf;
		}

		// Helper function to retrieve the 'self' pointer referenced by the 'node_pointer'
		static node_pointer self(node_pointer node_)
		{
			return &(**node_).pSelf;
		}


	public:
		// Helper function to retrieve the 'self' const pointer referenced by the end sentinel
		static const_node_pointer self_from_end(const_node_pointer node_)
		{
			return is_end_sentinel_of_empty_sublist(node_)
				? self(node_)
				: get_parent( self(node_) );
		}

		// Helper function to retrieve the 'self' pointer referenced by the end sentinel
		static node_pointer self_from_end(node_pointer node_)
		{
			return const_cast<node_pointer>(
				self_from_end(
					static_cast<const_node_pointer>(node_)
				));
		}

		// Helper function to retrieve the 'self' const pointer referenced by the rend sentinel
		static const_node_pointer self_from_rend(const_node_pointer node_)
		{
			return is_rend_sentinel_of_empty_sublist(node_)
				? self(node_)
				: get_parent(self(node_));
		}

		// Helper function to retrieve the 'self' pointer referenced by the rend sentinel
		static node_pointer self_from_rend(node_pointer node_)
		{
			return const_cast<node_pointer>(
				self_from_rend(
					static_cast<const_node_pointer>(node_)
				));
		}

		// Helper function to retrieve the 'self' const pointer referenced by the sentinel
		static const_node_pointer self_from_sentinel(const_node_pointer node_)
		{
			return is_sentinel_of_empty_sublist(node_)
				? self(node_)
				: get_parent(self(node_));
		}

		// Helper function to retrieve the 'self' pointer referenced by the sentinel
		static node_pointer self_from_sentinel(node_pointer node_)
		{
			return const_cast<node_pointer>(
				self_from_sentinel(
					static_cast<const_node_pointer>(node_)
				));
		}

		// Helper function to access the first child const pointer (or end sentinel if no children)
		static const_node_pointer get_begin(const_node_pointer node_)
		{
			return has_children(node_)
				? &(**node_).pREnd->pSelf
				: &(**node_).pEnd;
		}

		// Helper function to access the first child pointer (or end sentinel if no children)
		static node_pointer get_begin(node_pointer node_)
		{
			return const_cast<node_pointer>(
				get_begin(
					static_cast<const_node_pointer>(node_)
				));
		}

		// Helper function to access the child end sentinel const pointer
		static const_node_pointer get_end(const_node_pointer node_)
		{
			return &(**node_).pEnd;
		}

		// Helper function to access the child end sentinel pointer
		static node_pointer get_end(node_pointer node_)
		{
			return const_cast<node_pointer>(
				get_end(
					static_cast<const_node_pointer>(node_)
				));
		}

		// Helper function to access the last child const pointer (or rend sentinel if no children)
		static const_node_pointer get_rbegin(const_node_pointer node_)
		{
			return has_children(node_)
				? &(**node_).pEnd->pSelf
				: &(**node_).pREnd;
		}

		// Helper function to access the last child pointer (or rend sentinel if no children)
		static node_pointer get_rbegin(node_pointer node_)
		{
			return const_cast<node_pointer>(
				get_rbegin(
					static_cast<const_node_pointer>(node_)
				));
		}

		// Helper function to access the child rend sentinel const pointer
		static const_node_pointer get_rend(const_node_pointer node_)
		{
			return &(**node_).pREnd;
		}

		// Helper function to access the child rend sentinel pointer
		static node_pointer get_rend(node_pointer node_)
		{
			return const_cast<node_pointer>(
				get_rend(
					static_cast<const_node_pointer>(node_)
				));
		}


	public:
		// Returns the total number of elements
		static size_type get_size(const_node_pointer node_)
		{
			return (**node_).nSize;
		}

		// Returns the number of direct children of the node
		static size_type get_child_count(const_node_pointer node_)
		{
			return (**node_).nChildCount;
		}

		// Helper function to access the data const reference
		static const_reference data_ref(const_node_pointer node_)
		{
			return reinterpret_cast<const_reference>(
				(**node_).data
				);
		}

		// Helper function to access the data reference
		static reference data_ref(node_pointer node_)
		{
			return reinterpret_cast<reference>(
				(**node_).data
				);
		}

		// Helper function to access the data const pointer
		static const_pointer data_ptr(const_node_pointer node_)
		{
			return &data_ref(node_);
		}

		// Helper function to access the data pointer
		static pointer data_ptr(node_pointer node_)
		{
			return &data_ref(node_);
		}


	public:
		// Checks if the node has any children
		static bool has_children(const_node_pointer node_)
		{
			return (get_child_count(node_) > 0);
		}

		// Checks if a node is a end sentinel that indicating an empty list (points to itself)
		static bool is_end_sentinel_of_empty_sublist(const_node_pointer node_)
		{
			return (node_ == get_end(node_));
		}

		// Checks if a node is a rend sentinel that indicating an empty list (points to itself)
		static bool is_rend_sentinel_of_empty_sublist(const_node_pointer node_)
		{
			return (node_ == get_rend(node_));
		}

		// Checks if a node is a sentinel that indicating an empty list (points to itself)
		static bool is_sentinel_of_empty_sublist(const_node_pointer node_)
		{
			return is_end_sentinel_of_empty_sublist(node_)
				or is_rend_sentinel_of_empty_sublist(node_);
		}

		// Checks if nodes has same parent
		static bool is_same_parent(const_node_pointer first_, const_node_pointer second_)
		{
			return (get_parent(first_) == get_parent(second_));
		}

		// Checks if range is empty
		static bool is_empty_range(const_node_pointer begin_, const_node_pointer end_)
		{
			return (begin_ == end_);
		}

		// Check if the node is the sentinel root
		static bool is_root(const_node_pointer node_)
		{
			// Only sentinel root has nullptr parent (unlinked node has parent==pSelf)
			return (get_parent(node_) == nullptr);
		}

		// Check if node is sentinel (not points to itself)
		static bool is_sentinel(const_node_pointer node_)
		{
			return (node_ != self(node_));  // Any sentinel except root
		}

		// Check if the node is the first node of child list
		static bool is_begin(const_node_pointer node_)
		{
			return (node_ == self(prev_sibling_raw(node_)));
		}

		// Check if the node is the end sentinel
		static bool is_end(const_node_pointer node_)
		{
			return (node_ == get_end(node_))  // Case: sentinel of empty list
				or (node_ == next_sibling_raw(node_));
		}

		// Check if the node is the last node of child list
		static bool is_rbegin(const_node_pointer node_)
		{
			return (node_ == self(next_sibling_raw(node_)));
		}

		// Check if the node is the reverse end sentinel
		static bool is_rend(const_node_pointer node_)
		{
			return (node_ == get_rend(node_))  // Case: sentinel of empty list
				or (node_ == prev_sibling_raw(node_));
		}

		// Check if `descendant` is in the subtree under `parent`
		static bool is_descendant(const_node_pointer descendant_, const_node_pointer parent_) {
			for (auto it_ = descendant_; is_valid(it_); it_ = get_parent(it_)) {
				if (it_ == parent_) { return true; }
			}
			return false;
		}

		// Internal validation for node operations
		static bool is_valid(const_node_pointer node_)
		{
			return (node_ and *node_);
		}

		// Internal invalidation for node operations
		static bool is_not_valid(const_node_pointer node_)
		{
			return !is_valid(node_);
		}


	public:
		static void validate_source(const_node_pointer node_)
		{
			if (is_not_valid(node_) or is_sentinel(node_)) {
				throw std::invalid_argument("Attempted to access invalid element.");
			}
		}

		static void validate_destination(const_node_pointer node_)
		{
			if (is_not_valid(node_)) {
				throw std::invalid_argument("Attempted to access invalid element.");
			}
		}

		static void validate_dependency(const_node_pointer descendant_, const_node_pointer parent_)
		{
			if (is_descendant(descendant_, parent_)) {
				throw std::invalid_argument("Attempted to create a circular dependency.");
			}
		}

		static void validate_origin(const_node_pointer first_, const_node_pointer second_)
		{
			if (first_ != second_) {
				std::string error_msg =
					"Invalid iterator range provided. ";
					"Both 'begin' and 'end' iterators (or their underlying node pointers) ";
					"must belong to the same logical subtree or container for this operation. ";
					"For example, to specify a subtree, obtain both iterators from a common parent node "
					"(e.g., node.as_preorder().begin(), node.as_preorder().end()). ";
					"Ensure the range [first, second) is coherent within a single traversal view.";
				throw std::invalid_argument(error_msg);
			}
		}


	private:
		// @brief  Traverses range of nodes in a specified order, applying a unary operation to each node.
		//
		// @tparam TTraversePolicy  A callable object (functor or lambda) that takes (current_node, end_node)
		//         and returns the next node in the desired traversal order.
		// @tparam NodeTy_  The type of the node pointer (e.g., node_pointer, const_node_pointer).
		// @tparam UnOp_  A callable object (functor or lambda) that takes (node) and returns a boolean
		//         indicating whether traversal should continue (true) or halt (false).
		//
		// @param node_  The starting node pointer of the traversal range (inclusive).
		// @param end_  The ending node pointer of the traversal range (exclusive).
		// @param op_  An instance of the UnOp_ functor to be applied to each node.
		// @param policy_next  An instance of the TTraversePolicy functor defining the traversal order.
		//
		// @return true  If the traversal completed by reaching 'end' and the operation consistently returned true.
		// @return false  If the operation returned false at any point, halting the traversal prematurely.
		template<typename TTraversePolicy, typename NodeTy_, typename UnOp_>
		static bool for_each(NodeTy_ node_, NodeTy_ end_, UnOp_&& op_)
		{
			// Tracks whether traversal should continue
			bool condition_{ true };

			// Traverse while condition is true and iterator hasn't reached the end
			while (condition_ and node_ != end_) {
				condition_ = op_(node_);  // Apply the unary operation
				node_ = TTraversePolicy::policy_next(node_, end_);  // Move to the next node
			}
			// Return true if the loop completed and the iterator reached the end
			return condition_ and (node_ == end_);
		}

		// @brief  Traverses two ranges of nodes in parallel (lockstep), applying a binary operation to corresponding nodes.
		//
		// @tparam TTraversePolicy  A callable object (functor or lambda) that takes (current_node, end_node)
		//         and returns the next node in the desired traversal order.
		// @tparam LhsTy_  The type of the node pointer for the first sequence (e.g., node_pointer, const_node_pointer).
		// @tparam RhsTy_  The type of the node pointer for the second sequence (e.g., node_pointer, const_node_pointer).
		// @tparam BinOp_  A callable object (functor or lambda) that takes (lhs_node, rhs_node) and returns a boolean
		//         indicating whether traversal should continue (true) or halt (false).
		//
		// @param lhs_  The starting node pointer of the first traversal range (inclusive).
		// @param lend_  The ending node pointer of the first traversal range (exclusive).
		// @param rhs_  The starting node pointer of the second traversal range (inclusive).
		// @param rend_  The ending node pointer of the second traversal range (exclusive).
		// @param op_  An instance of the BinOp_ functor (binary operation) to be applied to corresponding nodes.
		// @param policy_next_  An instance of the TTraversePolicy functor defining the common traversal order for both trees.
		//
		// @return true  If both traversals completed by reaching their respective 'lend' and 'rend' simultaneously,
		//         and the operation consistently returned true.
		// @return false  If the operation returned false at any point, or if one tree ended prematurely
		//         (indicating a structural mismatch), halting the traversal.
		template<typename TTraversePolicy, typename LhsTy_, typename RhsTy_, typename BinOp_>
		static bool for_each(LhsTy_ lhs_, LhsTy_ lend_, RhsTy_ rhs_, RhsTy_ rend_, BinOp_&& op_)
		{
			// Tracks whether traversal should continue
			bool condition_{ true };

			// Traverse while condition is true and iterator hasn't reached the end
			while (condition_ and (lhs_ != lend_) and (rhs_ != rend_)) {
				condition_ = op_(lhs_, rhs_);  // Apply the binary operation
				lhs_ = TTraversePolicy::policy_next(lhs_, lend_);  // Move to the next node in lhs traversal
				rhs_ = TTraversePolicy::policy_next(rhs_, rend_);  // Move to the next node in rhs traversal
			}
			// Return true if the loop completed and the iterator reached the end
			return condition_ and (lhs_ == lend_) and (rhs_ == rend_);
		}

		// @brief Reverse-preorder traversal of subtree in lockstep, capturing node and applying op_ one step behind.
		//
		// @tparam TTraversePolicy  A callable object (functor or lambda) that takes (current_node)
		//         and returns the prev node in the desired traversal order.
		// @tparam NodeTy_  The type of the node pointer for the tree (e.g., node_pointer, const_node_pointer).
		// @tparam UnOp_  A callable object (functor or lambda) that takes (node) and returns a boolean
		//         indicating whether traversal should continue (true) or halt (false).
		//
		// @param node_  The starting node pointer of the traversal range (inclusive).
		// @param end_  The ending node pointer of the traversal range (exclusive).
		// @param op_  An instance of the UnOp_ functor to be applied to each node.
		//
		// @return true  If the traversal completed by reaching 'end' and the operation consistently returned true.
		// @return false  If the operation returned false at any point, halting the traversal prematurely.
		template<typename TTraversePolicy, typename NodeTy_, typename UnOp_>
		static bool for_each_reverse(NodeTy_ node_, NodeTy_ end_, UnOp_ op_)
		{
			// Empty range
			if (is_empty_range(node_, end_)) { return true; }

			// Tracks whether traversal should continue
			bool condition_{ true };

			// Move to first real node in reverse-preorder (skip sentinel)
			node_ = TTraversePolicy::policy_prev(node_);
			while (condition_ and (node_ != end_)) {
				auto captured_ = std::exchange(node_, TTraversePolicy::policy_prev(node_));  // Capture current node before stepping backwards
				condition_ = op_(captured_);  // Invoke op_ on the captured node
			}
			if (!condition_ or (node_ != end_)) { return false; }  // Stopped early by op_
			// Final root invocation (delayed semantics)
			return op_(node_);
		}

		// @brief Reverse-preorder traversal of two subtrees in lockstep, capturing nodes and applying op_ one step behind.
		//
		// @tparam TTraversePolicy  A callable object (functor or lambda) that takes (current_node)
		//         and returns the prev node in the desired traversal order.
		// @tparam LhsTy_  The type of the node pointer for the first sequence (e.g., node_pointer, const_node_pointer).
		// @tparam RhsTy_  The type of the node pointer for the second sequence (e.g., node_pointer, const_node_pointer).
		// @tparam BinOp_  A callable object (functor or lambda) that takes (lhs_node, rhs_node) and returns a boolean
		//         indicating whether traversal should continue (true) or halt (false).
		//
		// @param lhs_  The starting node pointer of the first traversal range (inclusive).
		// @param lend_  The ending node pointer of the first traversal range (exclusive).
		// @param rhs_  The starting node pointer of the second traversal range (inclusive).
		// @param rend_  The ending node pointer of the second traversal range (exclusive).
		// @param op_  An instance of the BinOp_ functor (binary operation) to be applied to corresponding nodes.
		// @param policy_next_  An instance of the TTraversePolicy functor defining the common traversal order for both trees.
		//
		// @return true  If both traversals completed by reaching their respective 'lend' and 'rend' simultaneously,
		//         and the operation consistently returned true.
		// @return false  If the operation returned false at any point, or if one tree ended prematurely
		//         (indicating a structural mismatch), halting the traversal.
		template<typename TTraversePolicy, typename LhsTy_, typename RhsTy_, typename BinOp_>
		static bool for_each_reverse(LhsTy_ lhs_, LhsTy_ lend_, RhsTy_ rhs_, RhsTy_ rend_, BinOp_&& op_)
		{
			// Both ranges empty (Success)
			if (is_empty_range(lhs_, lend_) and is_empty_range(rhs_, rend_)) { return true; }
			// One range empty, the other not (Failure/Mismatch)
			if (is_empty_range(lhs_, lend_) or is_empty_range(rhs_, rend_)) { return false; }

			// Tracks whether traversal should continue
			bool condition_{ true };

			// Move to first real nodes in reverse-preorder (skip sentinel)
			lhs_ = TTraversePolicy::policy_prev(lhs_);
			rhs_ = TTraversePolicy::policy_prev(rhs_);
			// Walk backwards until either op_ fails or one subtree is exhausted
			while (condition_ and (lhs_ != lend_) and (rhs_ != rend_)) {
				auto lcaptured_ = std::exchange(lhs_, TTraversePolicy::policy_prev(lhs_));  // Capture current nodes before stepping backwards
				auto rcaptured_ = std::exchange(rhs_, TTraversePolicy::policy_prev(rhs_));
				condition_ = op_(lcaptured_, rcaptured_);  // Invoke op_ on the captured nodes
			}
			if (!condition_ or (lhs_ != lend_) or (rhs_ != rend_)) { return false; }
			// Final root-pair invocation (delayed semantics)
			return op_(lhs_, rhs_);
		}

	};



	//=== Represents a forest-like container ===//
	template < typename T, typename TTraits = BasicTraits<T> >
	class Container
	{
	public:
		// Standard type aliases
		using self_type        = Container;
		using value_type       = typename TTraits::value_type;
		using pointer          = typename TTraits::pointer;
		using const_pointer    = typename TTraits::const_pointer;
		using reference        = typename TTraits::reference;
		using const_reference  = typename TTraits::const_reference;
		using difference_type  = typename TTraits::difference_type;
		using size_type        = typename TTraits::size_type;

	private:
		// Node management type aliases
		using Node                = typename NodeManager<self_type>;
		using node_type           = typename Node::node_type;
		using node_base           = typename Node::node_base;
		using node_pointer        = typename Node::node_pointer;
		using const_node_pointer  = typename Node::const_node_pointer;

		// Generic iterator types
		template <bool B, typename U> using generic_iterator  = Iterator<self_type, B, U>;
		template <typename U> using const_iterator            = Iterator<self_type, true, U>;
		template <typename U> using iterator                  = Iterator<self_type, false, U>;

	public:
		// Traverse policy aliases
		using PreorderTraversePolicy  = typename Node::PreorderTraversePolicy_;
		using FlatTraversePolicy      = typename Node::FlatTraversePolicy_;

		// Iterator types for traversing the container in flat (sibling) order
		using const_flat_iterator          = const_iterator<FlatTraversePolicy>;
		using flat_iterator                = iterator<FlatTraversePolicy>;
		using const_reverse_flat_iterator  = std::reverse_iterator<const_flat_iterator>;
		using reverse_flat_iterator        = std::reverse_iterator<flat_iterator>;

		// Iterator types for traversing the container in pre-order
		using const_preorder_iterator          = const_iterator<PreorderTraversePolicy>;
		using preorder_iterator                = iterator<PreorderTraversePolicy>;
		using const_reverse_preorder_iterator  = std::reverse_iterator<const_preorder_iterator>;
		using reverse_preorder_iterator        = std::reverse_iterator<preorder_iterator>;


	public:
		//=== Defines the common interface for providing sub-tree methods ===//
		template <typename TTraversePolicy>
		class PolicyView
		{
		public:
			// Standard type aliases
			using self_type       = PolicyView;
			using container_type  = Container<value_type, TTraits>;

			// Template aliases for iterators with a specified traversal policy
			using policy_iterator                = Iterator<Container, false, TTraversePolicy>;
			using const_policy_iterator          = Iterator<Container, true, TTraversePolicy>;
			using reverse_policy_iterator        = std::reverse_iterator<policy_iterator>;
			using const_reverse_policy_iterator  = std::reverse_iterator<const_policy_iterator>;

		private:
			// The pointer representing the current node in this view
			node_pointer pNode;

		public:
			// Costructor from raw pointer
			PolicyView(node_pointer node_) : pNode{ node_ }
			{
				Node::validate_source(node_);
			}

		public:
			// Returns a constant iterator to the first node in default traversal order
			const_policy_iterator cbegin() const
			{
				return const_policy_iterator(
					Node::get_begin(pNode)
				);
			}
			// Returns a constant iterator to the end sentinel
			const_policy_iterator cend() const
			{
				return const_policy_iterator(
					Node::get_end(pNode)
				);
			}
			// Returns a constant iterator to the first node in default traversal order
			const_policy_iterator begin() const
			{
				return cbegin();
			}
			// Returns a constant iterator to the end sentinel
			const_policy_iterator end() const
			{
				return cend();
			}
			// Returns an iterator to the first node in default traversal order
			policy_iterator begin()
			{
				return policy_iterator(
					static_cast<const self_type*>(this)->cbegin()
				);
			}
			// Returns an iterator to the end sentinel
			policy_iterator end()
			{
				return policy_iterator(
					static_cast<const self_type*>(this)->cend()
				);
			}

			// Returns a constant reverse iterator to the last node in default traversal order
			const_reverse_policy_iterator crbegin() const
			{
				return const_reverse_policy_iterator(cend());
			}
			// Returns a constant reverse iterator to the node before the first node in default traversal order
			const_reverse_policy_iterator crend() const
			{
				return const_reverse_policy_iterator(cbegin());
			}
			// Returns a constant reverse iterator to the last node in traversal order
			const_reverse_policy_iterator rbegin() const
			{
				return const_reverse_policy_iterator(end());
			}
			// Returns a constant reverse iterator to the node before the first node in default traversal order
			const_reverse_policy_iterator rend() const
			{
				return const_reverse_policy_iterator(begin());
			}
			// Returns a reverse iterator to the last node in default traversal order
			reverse_policy_iterator rbegin()
			{
				return reverse_policy_iterator(end());
			}
			// Returns a reverse iterator to the node before the first node in default traversal order
			reverse_policy_iterator rend()
			{
				return reverse_policy_iterator(begin());
			}

		public:
			// @brief  Removes all occurrences of a specified value from the collection.
			//
			// @param value_  The value to be removed.
			// @return  The number of elements removed.
			size_type remove(const_reference value_)
			{
				return Node::template remove_if<TTraversePolicy>(
					Node::get_begin(pNode), Node::get_end(pNode),
					[&value_](const_reference current_value_) { return (current_value_ == value_); }
				);
			}

			// @brief  Removes elements from the collection that satisfy a given predicate.
			//
			// @tparam UnPred_  A unary predicate type. It should be a callable that takes
			//                  an element of the collection as an argument and returns a boolean.
			// @param pr_  The unary predicate. Elements for which `pr_` returns `true` will be removed.
			// @return  The number of elements removed.
			template <typename UnPred_>
			size_type remove_if(UnPred_&& pr_)
			{
				return Node::template remove_if<TTraversePolicy>(
					Node::get_begin(pNode), Node::get_end(pNode), std::forward<UnPred_>(pr_)
				);
			}

			// @brief  Creates shallow copies of a range of nodes [begin, end) and inserts them into the container.
			//
			// @param where_  An iterator indicating the position before which the copied range will be inserted.
			// @param begin_  An iterator to the first node in the range to be shallow copied.
			// @param end_  An iterator to one past the last node in the range to be shallow copied.
			// @return  An `policy_iterator` to the first of the newly inserted (shallow-copied) nodes.
			// @throws  std::invalid_argument If `where_`, `begin_`, or `end_` are invalid iterators.
			policy_iterator copy(const_policy_iterator where_)
			{
				container_type::validate_destination(where_);
				return policy_iterator(
					Node::template copy<TTraversePolicy>(where_, Node::get_begin(pNode), Node::get_end(pNode))
				);
			}

		public:
			// Returns the number of direct children of the node
			size_type child_count() const
			{
				return Node::get_child_count(pNode);
			}

			// Returns the total number of elements of the subtree
			size_type size() const
			{
				return Node::get_size(pNode) - 1;
			}
			// Alias for size()
			size_type count() const
			{
				return size();
			}

			// Checks if the node has any children
			bool has_children() const
			{
				return Node::has_children(pNode);
			}
		};


	private:
		// Aliases for container operations mode type
		using FlatView      = PolicyView<FlatTraversePolicy>;
		using PreorderView  = PolicyView<PreorderTraversePolicy>;


	private:
		node_pointer pRoot{
			Node::self( new node_base{} )
		};


	private:
		template <bool B, typename U>
		static void validate_source(generic_iterator<B, U> it_)
		{
			Node::validate_source(it_.base());
		}

		template <bool B, typename U>
		static void validate_destination(generic_iterator<B, U> it_)
		{
			Node::validate_destination(it_.base());
		}

		template <bool B, typename U>
		static void validate_range(generic_iterator<B, U> first_, generic_iterator<B, U> second_)
		{
			Node::validate_origin(first_.orig(), second_.orig());
			Node::validate_destination(first_.base());
			Node::validate_destination(second_.base());
		}

		template <bool B, typename U>
		static void validate_dependency(generic_iterator<B, U> descendant_, generic_iterator<B, U> parent_)
		{
			Node::validate_dependency(descendant_.base(), parent_.base());
		}


	private:
		// Private constuctor from iterator to node
		template <typename U>
		explicit Container(iterator<U> it_)
		{
			validate_source(it_);
			Node::link(Node::get_end(pRoot), it_.base());
		}


	public:
		// Returns a const PolicyView wrapper for flat (sibling) traversal
		const FlatView as_flat() const
		{
			return FlatView{ pRoot };
		};
		// Returns a PolicyView wrapper for flat (sibling) traversal
		FlatView as_flat()
		{
			return FlatView{ pRoot };
		};
		// Alias for 'as_flat() const'
		const FlatView flat() const
		{
			return as_flat();
		};
		// Alias for 'as_flat()'
		FlatView flat()
		{
			return as_flat();
		};

		// Returns a const PolicyView wrapper for pre-order (depth-first) traversal
		const PreorderView as_preorder() const
		{
			return PreorderView{ pRoot };
		};
		// Returns a PolicyView wrapper for pre-order (depth-first) traversal
		PreorderView as_preorder()
		{
			return PreorderView{ pRoot };
		};
		// Alias for 'as_preorder() const'
		const PreorderView pre() const
		{
			return as_preorder();
		};
		// Alias for 'as_preorder()'
		PreorderView pre()
		{
			return as_preorder();
		};


	public:
		// Destructor: clears the entire tree
		~Container()
		{
			clear();
			delete static_cast<node_base*>(*pRoot);
		}

		// Default constructor
		Container() = default;

		// Copy constructor: performs a deep copy of the other tree
		Container(const self_type& other_)
		{
			*this = other_;  // Use copy assignment
		}

		// Move constructor: transfers ownership of the other tree's resources
		Container(self_type&& other_) noexcept
		{
			*this = std::move(other_);  // Use move assignment
		}

		// Constructor from an initializer list
		Container(value_type&& value_, std::initializer_list<self_type> init_) :
			Container(std::move(value_))
		{
			for (auto it = std::begin(init_); it != std::end(init_); ++it) {
				join(as_preorder().cbegin()().cend(), std::move(const_cast<self_type&>(*it)));
			}
		}

		// Constructor from an initializer list
		Container(std::initializer_list<self_type> init_)
		{
			for (auto it = std::begin(init_); it != std::end(init_); ++it) {
				join(as_preorder().cend(), std::move(const_cast<self_type&>(*it)));
			}
		}

		// Constructor taking a const_reference value
		explicit Container(const_reference value_) 
		{
			Node::link(Node::get_end(pRoot), Node::self(new node_type(value_)));
		}

		// Constructor taking an rvalue_type value
		explicit Container(value_type&& value_) noexcept
		{
			Node::link(Node::get_end(pRoot), Node::self(new node_type(std::move(value_))));
		}

		// Constructor from an initializer list
		explicit Container(std::initializer_list<value_type> init_)
		{
			*this = init_;  // Use initializer list assignment
		}

		// Copy assignment operator: clears the current container and deep copies the other tree
		self_type& operator =(const self_type& other_)
		{
			if (this == &other_) { return *this; }  // Handle self-assignment

			clear();
			if (!other_.empty()) {
				Node::template deep_copy<FlatTraversePolicy>(
					Node::get_end(pRoot),
					Node::get_begin(other_.pRoot), Node::get_end(other_.pRoot));
			}
			return *this;
		}

		// Move assignment operator: clears the current container and transfers ownership from the other tree
		self_type& operator =(self_type&& other_) noexcept
		{
			if (this == &other_) { return *this; }  // Handle self-assignment
			clear();
			std::swap(pRoot, other_.pRoot);
			return *this;
		}

		// Assignment operator from an initializer list
		self_type& operator =(std::initializer_list<value_type> init_)
		{
			clear();
			for (auto it = std::begin(init_); it != std::end(init_); ++it) {
				Node::link(Node::get_end(pRoot), Node::self(new node_type(*it)));
			}
			return *this;
		}


		// Equality operator for containers
		friend bool operator ==(const self_type& lhs_, const self_type& rhs_)
		{
			if (&lhs_ == &rhs_) { return true; }
			return Node::template deep_compare<FlatTraversePolicy>(
				Node::get_begin(lhs_.pRoot), Node::get_end(lhs_.pRoot),
				Node::get_begin(rhs_.pRoot), Node::get_end(rhs_.pRoot),
				std::equal_to<>()
			);
		}

		// Inequality operator for containers
		friend bool operator !=(const self_type& lhs_, const self_type& rhs_)
		{
			return !(lhs_ == rhs_);
		}

		// Prints the entire container structure
		friend std::ostream& operator <<(std::ostream& os_, const self_type& cont_)
		{
			return cont_.to_stream(os_);
		}


	public:
		// @brief  Appends one or more trees as children of the last node in a pre-order traversal.
		//
		// @tparam Trees  Variadic pack of rvalue references to tree types (self_type&&).
		// @param trees_  One or more tree objects to append. These are moved into the container.
		// @return  A reference to the current tree (*this) for method chaining.
		// @note  The insertion point is the end of the node preceding the end sentinel in a pre-order
		//        traversal, effectively adding children to the deepest, rightmost node.
		template<typename... Trees>
		self_type& append(Trees&&... trees_)
		{
			auto where_ = empty()  // As child of last in pre-order node
				? Node::get_end(pRoot)
				: Node::get_end(PreorderTraversePolicy::policy_prev(Node::get_end(pRoot)));

			([&](auto&& other_) {
				Node::template move<FlatTraversePolicy>(where_, Node::get_begin(other_.pRoot), Node::get_end(other_.pRoot));
				}(std::forward<Trees>(trees_)), ...);
			return *this;
		}

		// @brief  Inserts a single value by copy before the position indicated by 'where_'.
		//
		// @param where_  An iterator indicating the position before which the new node will be inserted.
		// @param value_  A constant reference to the value to be inserted.
		// @return  An `iterator<U>` to the newly inserted element.
		// @throws  std::invalid_argument If 'where_' is an invalid iterator.
		template <bool B, typename U>
		iterator<U> insert(generic_iterator<B, U> where_, const_reference value_)
		{
			validate_destination(where_);
			return iterator<U>(
				Node::link(where_.base(), Node::self(new node_type(value_)))
			);
		}

		// @brief  Inserts a single value by move before the position indicated by 'where_'.
		//
		// @param where_  An iterator indicating the position before which the new node will be inserted.
		// @param value_  An rvalue reference to the value to be moved and inserted.
		// @return  An `iterator<U>` to the newly inserted element.
		// @throws  std::invalid_argument If 'where_' is an invalid iterator.
		template <bool B, typename U>
		iterator<U> insert(generic_iterator<B, U> where_, value_type&& value_)
		{
			validate_destination(where_);
			return iterator<U>(
				Node::link(where_.base(), Node::self(new node_type(std::move(value_))))  // Uses move semantics
			);
		}

		// @brief  Inserts elements from an initializer list before the position indicated by 'where_'.
		//
		// @param where_  An iterator indicating the position before which the new nodes will be inserted.
		// @param init_  An `std::initializer_list` containing the values to be inserted.
		// @return  An `iterator<U>` to the first of the newly inserted elements.
		// @throws  std::invalid_argument If 'where_' is an invalid iterator.
		template <bool B, typename U>
		iterator<U> insert(generic_iterator<B, U> where_, std::initializer_list<value_type> init_)
		{
			validate_destination(where_);
			for (auto it = std::rbegin(init_); it != std::rend(init_); ++it) {
				where_ = Node::link(where_.base(), Node::self(new node_type(*it)));
			}
			return iterator<U>(where_);
		}

		// @brief  Constructs an element in place before the position indicated by 'where_'.
		//
		// @tparam Args  A variadic template parameter pack for the arguments to the `value_type` constructor.
		// @param where_  An iterator indicating the position before which the new node will be inserted.
		// @param args  Arguments perfectly forwarded to the constructor of `value_type`.
		// @return  An `iterator<U>` to the newly emplaced element.
		// @throws  std::invalid_argument If 'where_' is an invalid iterator.
		template <bool B, typename U, typename... Args>
		iterator<U> emplace(generic_iterator<B, U> where_, Args&&... args)
		{
			validate_destination(where_);
			return iterator<U>(
				Node::link(where_.base(), Node::self(new node_type(std::forward<Args>(args)...)))
			);
		}

		// @brief  Creates a shallow copy of a single node and inserts it into the container.
		//
		// @param where_  An iterator indicating the position before which the copied node will be inserted.
		// @param it_  An iterator pointing to the node whose element will be copied.
		// @return  An `iterator<U>` to the newly inserted node.
		// @throws  std::invalid_argument If `where_` or `it_` are invalid iterators.
		template <bool Bf, bool Bs, typename U>
		iterator<U> copy(generic_iterator<Bf, U> where_, generic_iterator<Bs, U> it_)
		{
			validate_destination(where_);
			validate_source(it_);
			return iterator<U>(
				Node::shallow_copy(where_.base(), it_.base())
			);
		}

		// @brief  Creates shallow copies of a range of nodes and inserts them into the container.
		//
		// @param where_  An iterator indicating the position before which the copied range will be inserted.
		// @param begin_  An iterator to the first node in the range to be copied.
		// @param end_  An iterator to one past the last node in the range to be copied.
		// @return  An `iterator<U>` to the first of the newly inserted nodes.
		// @throws  std::invalid_argument If `where_`, `begin_`, or `end_` are invalid iterators.
		template <bool Bf, bool Bs, typename U>
		iterator<U> copy(generic_iterator<Bf, U> where_, generic_iterator<Bs, U> begin_, generic_iterator<Bs, U> end_)
		{
			validate_destination(where_);
			validate_range(begin_, end_);
			return iterator<U>(
				Node::template shallow_copy<U>(where_.base(), begin_.base(), end_.base())
			);
		}

		// @brief  Creates shallow copies of elements from a generic input range and inserts them.
		//
		// @param where_  An iterator from this container indicating the position before which the copied range will be inserted.
		// @param begin_  A generic input iterator to the first element in the source range.
		// @param end_  A generic input iterator to one past the last element in the source range.
		// @return  An `iterator<U>` to the first of the newly inserted (shallow-copied) nodes.
		// @throws  std::invalid_argument If `where_` is an invalid iterator.
		template <bool B, typename InputIt, typename U>
		std::enable_if_t<!std::is_base_of_v<generic_iterator<B, U>, InputIt>, iterator<U>>
			copy(generic_iterator<B, U> where_, InputIt begin_, InputIt end_)
		{
			validate_destination(where_);
			if (begin_ == end_) { return iterator<U>{ where_ }; }
			iterator<U> captured_ = insert(where_, *begin_);
			for (++begin_; begin_ != end_; ++begin_) {
				insert(where_.base(), *begin_);
			}
			return captured_;
		}

		// @brief  Creates a deep copy of a single node and inserts it into the container.
		//
		// @param where_  An iterator indicating the position before which the copied node will be inserted.
		// @param it_  An iterator pointing to the node whose element will be copied.
		// @return  An `iterator<U>` to the newly inserted node.
		// @throws  std::invalid_argument If `where_` or `it_` are invalid iterators.
		template <bool Bf, bool Bs, typename U>
		iterator<U> deep_copy(generic_iterator<Bf, U> where_, generic_iterator<Bs, U> it_)
		{
			validate_destination(where_);
			validate_source(it_);
			return iterator<U>(
				Node::deep_copy(where_.base(), it_.base())
			);
		}

		// @brief  Creates deep copies of a range of nodes and inserts them into the container.
		//
		// @param where_  An iterator indicating the position before which the copied range will be inserted.
		// @param begin_  An iterator to the first node in the range to be copied.
		// @param end_  An iterator to one past the last node in the range to be copied.
		// @return  An `iterator<U>` to the first of the newly inserted nodes.
		// @throws  std::invalid_argument If `where_`, `begin_`, or `end_` are invalid iterators.
		template <bool Bf, bool Bs, typename U>
		std::enable_if_t<std::is_same_v<U, FlatTraversePolicy>, iterator<U>>
			deep_copy(generic_iterator<Bf, U> where_, generic_iterator<Bs, U> begin_, generic_iterator<Bs, U> end_)
		{
			validate_destination(where_);
			validate_range(begin_, end_);
			return iterator<U>(
				Node::template deep_copy<U>(where_.base(), begin_.base(), end_.base())
			);
		}

		// @brief  Moves a single node to a new position within the containers of same type.
		//
		// @param where_  An iterator indicating the position before which 'it_' will be inserted.
		// @param it_  An iterator pointing to the node to be moved.
		// @return  An iterator to the moved node in its new position.
		// @throws  std::invalid_argument If 'where_' is a descendant of 'it_', preventing self-containment.
		template <bool Bf, bool Bs, typename U>
		std::enable_if_t<std::is_same_v<U, FlatTraversePolicy>, iterator<U>>
			move(generic_iterator<Bf, U> where_, generic_iterator<Bs, U> it_)
		{
			validate_destination(where_);
			validate_source(it_);
			validate_dependency(where_, it_);
			return iterator<U>(
				Node::move(where_.base(), it_.base())
			);
		}

		// @brief  Moves a range of nodes to a new position within the container.
		//
		// @param where_  An iterator indicating the position before which the range will be inserted.
		// @param begin_  An iterator to the first node in the range to be moved.
		// @param end_  An iterator to one past the last node in the range to be moved.
		// @return  An iterator to the first of the newly moved nodes in their new position.
		template <bool Bf, bool Bs, typename U>
		std::enable_if_t<std::is_same_v<U, FlatTraversePolicy>, iterator<U>>
			move(generic_iterator<Bf, U> where_, generic_iterator<Bs, U> begin_, generic_iterator<Bs, U> end_)
		{
			validate_destination(where_);
			validate_range(begin_, end_);
			return iterator<U>(
				Node::template move<U>(where_.base(), begin_.base(), end_.base())
			);
		}

		// @brief  Joins the contents of 'other_' (rvalue reference) into this container.
		//
		// @param where_  An iterator indicating the position before which the nodes from 'other_' will be inserted.
		// @param other_  An rvalue reference to the container whose contents will be moved.
		// @return  An iterator to the first of the newly joined nodes in this container,
		//          or 'where_' if 'other_' was empty or the operation failed.
		template <bool B, typename U>
		iterator<U> join(generic_iterator<B, U> where_, self_type&& other_)
		{
			validate_destination(where_);
			if (this == &other_) { return iterator<U>(where_); }  // Cannot join a container to itself
			if (other_.empty()) { return iterator<U>(where_); }   // Nothing to join if other is empty
			return iterator<U>(
				Node::template move<FlatTraversePolicy>(
					where_.base(), Node::get_begin(other_.pRoot), Node::get_end(other_.pRoot)
				));
		}

		// @brief  Joins the contents of 'other_' (lvalue reference) into this container.
		//
		// @param where_  An iterator indicating the position before which the nodes from 'other_' will be inserted.
		// @param other_  An lvalue reference to the container whose contents will be moved.
		// @return  An iterator to the first of the newly joined nodes in this container.
		template <bool B, typename U>
		iterator<U> join(generic_iterator<B, U> where_, self_type& other_)
		{
			return join(where_, std::move(other_));  // Call the move version
		}

		// @brief  Unjoins the subtree rooted at the node indicated by 'it_'.
		//
		// @param it_  An iterator to the node to be unjoined.
		// @return  A new container (self_type) containing the unjoined node.
		template <bool B, typename U>
		self_type unjoin(generic_iterator<B, U> it_)
		{
			validate_source(it_);
			Node::unlink(it_.base());
			return self_type{ it_ };
		}

		// @brief  Compares two individual nodes using a custom binary predicate.
		//
		// @tparam BinPred_  The type of the binary predicate for comparison.
		// @param first_  An iterator to the first node for comparison.
		// @param second_  An iterator to the second node for comparison.
		// @param is_equal_  A binary predicate that returns true if two elements are considered equal (defaults to std::equal_to<>()).
		// @return  True if the 'first_' and 'second_' are element-wise equal, false otherwise.
		template <bool Bf, bool Bs, typename U, typename BinPred_ = std::equal_to<>>
		bool compare(generic_iterator<Bf, U> first_, generic_iterator<Bs, U> second_, BinPred_&& equal_ = {})
		{
			validate_source(first_);
			validate_source(second_);
			return Node::shallow_compare(
				first_.base(), second_.base(), std::forward<BinPred_>(equal_)
			);
		}

		// @brief  Compares two ranges of nodes using a custom binary predicate.
		//
		// @tparam BinPred_  The type of the binary predicate for comparison.
		// @param first_begin_  An iterator to the beginning of the first range.
		// @param first_end_  An iterator to the end of the first range.
		// @param second_begin_  An iterator to the beginning of the second range.
		// @param second_end_  An iterator to the end of the second range.
		// @param is_equal_  A binary predicate that returns true if two elements are considered equal (defaults to std::equal_to<>()).
		// @return  True if the two ranges are element-wise equal, false otherwise.
		template <bool Bf, bool Bs, typename U, typename BinPred_ = std::equal_to<>>
		bool compare(generic_iterator<Bf, U> first_begin_, generic_iterator<Bf, U> first_end_,
			generic_iterator<Bs, U> second_begin_, generic_iterator<Bs, U> second_end_, BinPred_&& is_equal_ = {})
		{
			validate_range(first_begin_, first_end_);
			validate_range(second_begin_, second_end_);
			return Node::template shallow_compare<U>(
				first_begin_.base(), first_end_.base(), second_begin_.base(), second_end_.base(), std::forward<BinPred_>(is_equal_)
			);
		}

		// @brief  Compares two individual nodes (and their respective subtrees/sibling ranges).
		//
		// @tparam BinPred_  The type of the binary predicate for comparison.
		// @param first_  An iterator to the first node for comparison.
		// @param second_  An iterator to the second node for comparison.
		// @param is_equal_  A binary predicate that returns true if two elements are considered equal (defaults to std::equal_to<>()).
		// @return  True if the "ranges" rooted at 'first_' and 'second_' are element-wise equal, false otherwise.
		template <bool Bf, bool Bs, typename U, typename BinPred_ = std::equal_to<>>
		std::enable_if_t<std::is_same_v<U, FlatTraversePolicy>, bool>
			deep_compare(generic_iterator<Bf, U> first_, generic_iterator<Bs, U> second_,
			BinPred_&& equal_ = {})
		{
			validate_source(first_);
			validate_source(second_);
			return Node::deep_compare(
				first_.base(), second_.base(), std::forward<BinPred_>(equal_)
			);
		}

		// @brief  Compares two ranges of nodes using a custom binary predicate.
		//
		// @tparam BinPred_  The type of the binary predicate for comparison.
		// @param first_begin_  An iterator to the beginning of the first range.
		// @param first_end_  An iterator to the end of the first range.
		// @param second_begin_  An iterator to the beginning of the second range.
		// @param second_end_  An iterator to the end of the second range.
		// @param is_equal_  A binary predicate that returns true if two elements are considered equal (defaults to std::equal_to<>()).
		// @return  True if the two ranges are element-wise equal according to 'is_equal_', false otherwise.
		template <bool Bf, bool Bs, typename U, typename BinPred_ = std::equal_to<>>
		std::enable_if_t<std::is_same_v<U, FlatTraversePolicy>, bool>
			deep_compare(generic_iterator<Bf, U> first_begin_, generic_iterator<Bf, U> first_end_,
			generic_iterator<Bs, U> second_begin_, generic_iterator<Bs, U> second_end_, BinPred_&& is_equal_ = {})
		{
			validate_range(first_begin_, first_end_);
			validate_range(second_begin_, second_end_);
			return Node::template deep_compare<U>(
				first_begin_.base(), first_end_.base(), second_begin_.base(), second_end_.base(), std::forward<BinPred_>(is_equal_)
			);
		}

		// @brief  Swaps the positions of two nodes within the container structure.
		//
		// @param first_  An iterator pointing to the first node to be swapped.
		// @param second_  An iterator pointing to the second node to be swapped.
		// @throws  std::invalid_argument If either `first_` or `second_` is an invalid iterator
		//          or points to a sentinel node (e.g., the end iterator).
		template <bool Bf, bool Bs, typename U>
		void swap(generic_iterator<Bf, U> first_, generic_iterator<Bs, U> second_)
		{
			validate_source(first_);
			validate_source(second_);
			Node::swap_nodes(first_.base(), second_.base());
		}

		// @brief  Removes the node (and its entire subtree) indicated by the iterator.
		//
		// @param it_  An iterator pointing to the node to be removed.
		// @return  An iterator to the element immediately following the removed node (or the end iterator if no such element).
		// @throws  std::invalid_argument If `it_` is an invalid iterator or points to a sentinel node.
		template <bool B, typename U>
		iterator<U> remove(generic_iterator<B, U> it_)
		{
			validate_source(it_);
			return iterator<U>(
				Node::remove(it_.base())
			);
		}

		// @brief  Removes all nodes from the container whose data matches the given value.
		//
		// @param value_  The constant reference to the value to be removed from the container.
		// @return  The total `size_type` count of elements from the subtrees of all removed nodes.
		// @throws  std::invalid_argument If either `begin_` or `end_` is an invalid iterator.
		template <bool B, typename U>
		size_type remove(generic_iterator<B, U> begin_, generic_iterator<B, U> end_, const_reference value_)
		{
			validate_range(begin_, end_);
			return Node::template remove_if<U>(
				begin_.base(), end_.base(),
				[&value_](const_reference current_value_) {
					return (current_value_ == value_);
				}
			);
		}

		// @brief  Removes elements from the container based on a user-provided predicate.
		//
		// @tparam UnaryPred_  The type of the unary predicate. This type must be
		//                     a callable object (function, lambda, functor).
		// @param pr_  The unary predicate to apply to each node's data.
		// @return  The total `size_type` count of elements from the subtrees of all removed nodes.
		template <bool B, typename U, typename UnPred_>
		size_type remove_if(generic_iterator<B, U> begin_, generic_iterator<B, U> end_, UnPred_&& pr_)
		{
			validate_range(begin_, end_);
			return Node::template remove_if<U>(
				begin_.base(), end_.base(), std::forward<UnPred_>(pr_)
			);
		}

		// @brief  Clears the subtree or range rooted at the node indicated by 'it_'.
		//
		// @param it_  An iterator to the node marking the beginning of the range to be cleared.
		// @throws  std::invalid_argument If `it_` is an invalid iterator or points to a sentinel node.
		template <bool B, typename U>
		void clear(generic_iterator<B, U> it_)
		{
			validate_source(it_);
			Node::template remove_if<U>(
				Node::get_begin(it_.base()), Node::get_end(it_.base()),
				[](auto) { return true; }
			);
		}


	public:
		// Clears the entire container
		void clear()
		{
			Node::template remove_if<typename Node::PreorderTraversePolicy_>(
				Node::get_begin(pRoot), Node::get_end(pRoot),
				[](auto) { return true; }
			);
		}

		// Returns the total number of nodes in the container
		size_type size() const
		{
			return Node::get_size(pRoot) -1;  // Except root itself
		}
		// Alias for size()
		size_type count() const
		{
			return size();
		}

		// Returns the number of top-level sub-trees
		size_type child_count() const
		{
			return Node::get_child_count(pRoot);
		}

		// Checks if the container is empty
		bool empty() const
		{
			return !Node::has_children(pRoot);
		}

		// Outputs the entire container structure in a formatted manner to the given stream
		std::ostream& to_stream(std::ostream& os_) const
		{
			return Node::formatted_stream(os_, pRoot);
		}

	};



	//=== Iterator template for traversing the OutTree ===//
	template <typename TContainer, bool Const, typename TTraversePolicy>
	class Iterator
	{
	public:
		// Standard type aliases
		using container_type     = typename TContainer::self_type;
		using self_type          = Iterator;
		using iterator_category  = std::bidirectional_iterator_tag;
		using value_type         = typename container_type::value_type;
		using pointer            = typename container_type::pointer;
		using const_pointer      = typename container_type::const_pointer;
		using reference          = typename container_type::reference;
		using const_reference    = typename container_type::const_reference;
		using difference_type    = typename container_type::difference_type;
		using size_type          = typename container_type::size_type;

		// Traverse policy aliases
		using PreorderTraversePolicy  = typename container_type::PreorderTraversePolicy;
		using FlatTraversePolicy      = typename container_type::FlatTraversePolicy;

		// Iterator types for traversing the container in TraversePolicy defined order
		using iterator                = Iterator<TContainer, false, TTraversePolicy>;
		using const_iterator          = Iterator<TContainer, true, TTraversePolicy>;
		using reverse_iterator        = std::reverse_iterator<iterator>;
		using const_reverse_iterator  = std::reverse_iterator<const_iterator>;


	private:
		// Node management type aliases
		using Node                  = typename NodeManager<container_type>;
		using node_type             = typename Node::node_type;
		using node_pointer          = typename Node::node_pointer;
		using const_node_pointer    = typename Node::const_node_pointer;

		// Helper aliases
		using PolicyView  = typename container_type::template PolicyView<TTraversePolicy>;


	private:
		// Friend declarations
		template <typename, bool, typename> friend class Iterator;
		friend class container_type::self_type;


	private:
		node_pointer pNode;  // Pointer to a node's pSelf
		const_node_pointer pOrigin;  // Pointer to source node


	private:
		void validate_source() const
		{
			Node::validate_source(pNode);
		}

		void validate_destination() const
		{
			Node::validate_destination(pNode);
		}


	private:
		// Private constructor from a node pointer to an iterator
		explicit Iterator(node_pointer node_)
		{
			pNode = node_;
			pOrigin = Node::is_sentinel(pNode)
				? Node::self_from_sentinel(pNode)
				: Node::get_parent(pNode);
		}

		// Private constructor from a const node/source pointers to an iterator
		explicit Iterator(node_pointer node_, const_node_pointer source_)
		{
			pNode = node_;
			pOrigin = source_;
		}

		// Private constructor from a const iterator type to an iterator
		template < bool B = Const, typename = std::enable_if_t<!B> >
		explicit Iterator(const Iterator<TContainer, true, TTraversePolicy>& other_) :
			pNode(other_.pNode), pOrigin(other_.pOrigin)
		{}


		// Private assignment operator from a const node pointer to an iterator
		self_type& operator =(node_pointer node_)
		{
			pNode = node_;
			return *this;
		}

		// Private conversion operator from iterator to node pointer
		explicit operator node_pointer()
		{
			return pNode;
		}

		// Outputs the entire sub-tree structure in a formatted manner to the given stream
		std::ostream& to_stream(std::ostream& os_) const
		{
			return Node::formatted_stream(os_, pNode);
		}


		// Returns the underlying node const pointer
		const_node_pointer base() const
		{
			return pNode;
		}
		// Returns the underlying node pointer
		node_pointer base()
		{
			return pNode;
		}

		// Returns the underlying source const pointer
		const_node_pointer orig() const
		{
			return pOrigin;
		}


	public:
		// Destructor
		~Iterator() = default;


		// Default constructor (creates a null iterator)
		explicit Iterator(std::nullptr_t = 0) :
			pNode{}, pOrigin{}
		{}

		// Private assignment operator from a const node pointer to an iterator
		self_type& operator =(std::nullptr_t) const
		{
			pNode = pOrigin = nullptr;
			return *this;
		}

		
		// Implicit Copy c-tor: non-const to const_iterator (same TraversePolicy) and const to const_iterator (same TraversePolicy)
		template < bool B = Const, bool C, typename = std::enable_if_t<B> >
		Iterator(const Iterator<TContainer, C, TTraversePolicy>& other_) :
			pNode(other_.pNode), pOrigin(other_.pOrigin) {}

		// Explicit Copy c-tor: any iterator type to a const_iterator (different TraversePolicy)
		template < bool B = Const, bool C, typename T, typename = std::enable_if_t<B> >
		explicit Iterator(const Iterator<TContainer, C, T>& other_) :
			pNode(other_.pNode), pOrigin(other_.pOrigin) {}


		// Implicit Move c-tor: non-const to const_iterator (same TraversePolicy) or const to const_iterator (same TraversePolicy)
		template < bool B = Const, bool C, typename = std::enable_if_t<B> >
		Iterator(Iterator<TContainer, C, TTraversePolicy>&& other_) noexcept :
			pNode(std::exchange(other_.pNode, nullptr)), pOrigin(std::exchange(other_.pOrigin, nullptr)) {}

		// Explicit Move c-tor: any iterator type to a const_iterator (different TraversePolicy)
		template < bool B = Const, bool C, typename T, typename = std::enable_if_t<B> >
		explicit Iterator(Iterator<TContainer, C, T>&& other_) noexcept :
			pNode(std::exchange(other_.pNode, nullptr)), pOrigin(std::exchange(other_.pOrigin, nullptr)) {}


		// Copy-assignment op: non-const to const_iterator (different TraversePolicy)
		template < bool B = Const, bool C, typename T, typename = std::enable_if_t<B> >
		self_type& operator =(const Iterator<TContainer, C, T>& other_)
		{
			pNode = other_.pNode;
			pOrigin = other_.pOrigin;
			return *this;
		}

		// Move-assignment op: non-const to const_iterator (different TraversePolicy)
		template < bool B = Const, bool C, typename T, typename = std::enable_if_t<B> >
		self_type& operator =(Iterator<TContainer, C, T>&& other_) noexcept
		{
			pNode = std::exchange(other_.pNode, nullptr);
			pOrigin = std::exchange(other_.pOrigin, nullptr);
			return *this;
		}



		// Copy c-tor, any mutable iterator type to mutable iterator
		template < bool B = Const, typename = std::enable_if_t<!B> >
		Iterator(const Iterator<TContainer, false, TTraversePolicy>& other_) :
			pNode(other_.pNode), pOrigin(other_.pOrigin) {}

		// Copy c-tor, any mutable iterator type to mutable iterator
		template < bool B = Const, typename T, typename = std::enable_if_t<!B> >
		explicit Iterator(const Iterator<TContainer, false, T>& other_) :
			pNode(other_.pNode), pOrigin(other_.pOrigin) {}


		// Move c-tor, any mutable iterator type to mutable iterator
		template < bool B = Const, typename = std::enable_if_t<!B> >
		Iterator(Iterator<TContainer, false, TTraversePolicy>&& other_) noexcept :
			pNode(std::exchange(other_.pNode, nullptr)), pOrigin(std::exchange(other_.pOrigin, nullptr)) {}

		// Move c-tor, any mutable iterator type to mutable iterator
		template < bool B = Const, typename T, typename = std::enable_if_t<!B> >
		explicit Iterator(Iterator<TContainer, false, T>&& other_) noexcept :
			pNode(std::exchange(other_.pNode, nullptr)), pOrigin(std::exchange(other_.pOrigin, nullptr)) {}


		// Copy-assigment op, any mutable iterator type to mutable iterator
		template < bool B = Const, typename T, typename = std::enable_if_t<!B> >
		self_type& operator =(const Iterator<TContainer, false, T>& other_)
		{
			pNode = other_.pNode;
			pOrigin = other_.pOrigin;
			return *this;
		}

		// Move-assigment op, any mutable iterator type to mutable iterator
		template < bool B = Const, typename T, typename = std::enable_if_t<!B> >
		self_type& operator =(Iterator<TContainer, false, T>&& other_) noexcept
		{
			pNode = std::exchange(other_.pNode, nullptr);
			pOrigin = std::exchange(other_.pOrigin, nullptr);
			return *this;
		}


	public:
		// Pre-decrement operator (moves to the previous node in traversal order)
		self_type& operator --()
		{
			validate_destination();
			pNode = TTraversePolicy::policy_prev(pNode);
			return *this;
		}
		// Pre-increment operator (moves to the next node in traversal order)
		self_type& operator ++()
		{
			validate_destination();
			pNode = TTraversePolicy::policy_next(pNode, Node::get_end(pOrigin));
			return *this;
		}

		// Post-decrement operator
		self_type operator --(int)
		{
			self_type captured_(*this);
			--*this;
			return captured_;
		}
		// Post-increment operator
		self_type operator ++(int)
		{
			self_type captured_(*this);
			++*this;
			return captured_;
		}

		// Compound assignment for subtraction (iterates backward)
		self_type& operator -=(size_type value_)
		{
			while (value_--) { --*this; }
			return *this;
		}
		// Compound assignment for addition (iterates forward)
		self_type& operator +=(size_type value_)
		{
			while (value_--) { ++*this; }
			return *this;
		}

		// Subtraction operator (returns a new iterator)
		friend self_type operator -(self_type lhs_, size_type value_)
		{
			return lhs_ -= value_;
		}
		// Addition operator (returns a new iterator)
		friend self_type operator +(self_type lhs_, size_type rhs_)
		{
			return lhs_ += rhs_;
		}

		// Subtraction operator (returns a difference type)
		friend difference_type operator -(self_type lhs_, self_type rhs_)
		{
			return std::distance(lhs_, rhs_);
		}


		// Dereference operator (returns a reference to the stored value)
		const_reference operator *() const
		{
			return data_ref();
		}
		// Dereference operator (returns a reference to the stored value)
		reference operator *()
		{
			return data_ref();
		}

		// Dereference operator (returns a pointer to the stored value)
		const_pointer operator ->() const
		{
			return data_ptr();
		}
		// Dereference operator (returns a pointer to the stored value)
		pointer operator ->()
		{
			return data_ptr();
		}


		// Equality operator for iterators
		template < typename C_Type, bool B_LHS, typename T_LHS, bool B_RHS, typename T_RHS >
		friend bool operator ==(const Iterator<C_Type, B_LHS, T_LHS>& lhs_, const Iterator<C_Type, B_RHS, T_RHS>& rhs_);

		// Inequality operator for iterators
		template < typename C_Type, bool B_LHS, typename T_LHS, bool B_RHS, typename T_RHS >
		friend bool operator !=(const Iterator<C_Type, B_LHS, T_LHS>& lhs_, const Iterator<C_Type, B_RHS, T_RHS>& rhs_);


		// Outputs the entire sub-tree structure in a formatted manner to the given stream
		friend std::ostream& operator <<(std::ostream& os_, const self_type& it_)
		{
			return it_.to_stream(os_);
		}


	public:
		// Calling the const ContainerOps for the current node
		const PolicyView operator ()() const
		{
			return PolicyView{ pNode };
		}
		// Calling the ContainerOps for the current node
		PolicyView operator ()()
		{
			return PolicyView{ pNode };
		}


	public:
		// Returns a const iterator to the parent node
		const_iterator parent() const
		{
			if (Node::is_not_valid(pNode) or Node::is_sentinel(pNode)) {
				throw std::invalid_argument("Attempted to access invalid element.");
			}
			auto parent_ = Node::get_parent(pNode);
			return const_iterator(
				(Node::is_root(parent_) ? nullptr : parent_),
				pOrigin
			);
		}
		// Returns an iterator to the parent node
		iterator parent()
		{
			return iterator(
				static_cast<const self_type*>(this)->parent()
			);
		}

		// Returns a const iterator to the previous sibling node
		const_iterator prev_flat() const
		{
			if (Node::is_not_valid(pNode)) {
				throw std::invalid_argument("Attempted to access invalid element.");
			}
			if (Node::is_begin(pNode) or Node::is_rend(pNode)) {
				throw std::out_of_range("Attempted to access element out of bounds.");
			}
			return const_iterator(
				Node::FlatTraversePolicy_::policy_prev(pNode),
				pOrigin
			);
		}
		// Returns an iterator to the previous sibling node
		iterator prev_flat()
		{
			return iterator(
				static_cast<const self_type*>(this)->prev_flat()
			);
		}

		// Returns a const iterator to the next sibling node
		const_iterator next_flat() const
		{
			if (Node::is_not_valid(pNode)) {
				throw std::invalid_argument("Attempted to access invalid element.");
			}
			if (Node::is_end(pNode) or Node::is_rend(pNode)) {
				throw std::out_of_range("Attempted to access element out of bounds.");
			}
			return const_iterator(
				Node::FlatTraversePolicy_::policy_next(pNode),
				pOrigin
			);
		}
		// Returns an iterator to the next sibling node
		iterator next_flat()
		{
			return iterator(
				static_cast<const self_type*>(this)->next_flat()
			);
		}

		// Returns a const iterator to the previous element in pre-order traversal
		const_iterator prev_preorder() const
		{
			if (Node::is_not_valid(pNode)) {
				throw std::invalid_argument("Attempted to access invalid element.");
			}
			if (Node::is_rend(pNode)
				or (Node::is_begin(pNode) and Node::is_root( Node::get_parent(pNode) ))) {
				throw std::out_of_range("Attempted to access element out of bounds.");
			}
			return const_iterator(
				Node::PreorderTraversePolicy_::policy_prev(pNode),
				pOrigin
			);
		}
		// Returns an iterator to the previous element in pre-order traversal
		iterator prev_preorder()
		{
			return iterator(
				static_cast<const self_type*>(this)->prev_preorder()
			);
		}

		// Returns a const iterator to the next element in pre-order traversal
		const_iterator next_preorder() const
		{
			if (Node::is_not_valid(pNode)) {
				throw std::invalid_argument("Attempted to access invalid element.");
			}
			if (Node::is_end(pNode)) {
				throw std::out_of_range("Attempted to access element out of bounds.");
			}
			return const_iterator(
				Node::PreorderTraversePolicy_::policy_next(pNode, Node::get_end(pOrigin)),
				pOrigin
			);
		}
		// Returns an iterator to the next element in pre-order traversal
		iterator next_preorder()
		{
			return iterator(
				static_cast<const self_type*>(this)->next_preorder()
			);
		}

/*
	public:
		// Returns the number of direct children of the node
		size_type child_count() const
		{
			if (Node::is_not_valid(pNode) or Node::is_sentinel(pNode)) {
				throw std::invalid_argument("Attempted to access invalid element.");
			}
			return Node::get_child_count(pNode);
		}

		// Returns the total number of elements of the node
		size_type size() const
		{
			if (Node::is_not_valid(pNode) or Node::is_sentinel(pNode)) {
				throw std::invalid_argument("Attempted to access invalid element.");
			}
			return Node::get_size(pNode);
		}
		// Alias for size()
		size_type count() const
		{
			return size();
		}

		// Checks if the node has any children
		bool has_children() const
		{
			if (Node::is_not_valid(pNode) or Node::is_sentinel(pNode)) {
				throw std::invalid_argument("Attempted to access invalid element.");
			}
			return Node::has_children(pNode);
		}
*/

	public:
		// Returns a const reference to the node's data
		const_reference data_ref() const
		{
			validate_source();
			return reinterpret_cast<const_reference>( Node::data_ref(pNode) );
		}
		// Returns a reference to the node's data
		reference data_ref()
		{
			return const_cast<reference>(
				static_cast<const self_type*>(this)->data_ref()
				);
		}

		// Returns a const pointer to the node's data
		const_pointer data_ptr() const
		{
			return &data_ref();
		}
		// Returns a pointer to the node's data
		pointer data_ptr()
		{
			return const_cast<pointer>(
				static_cast<const self_type*>(this)->data_ptr()
				);
		}

	};



	// Equality operator for iterators
	template < typename C_Type, bool B_LHS, typename T_LHS, bool B_RHS, typename T_RHS >
	bool operator ==(const Iterator<C_Type, B_LHS, T_LHS>& lhs_, const Iterator<C_Type, B_RHS, T_RHS>& rhs_)
	{
		return (lhs_.pNode == rhs_.pNode);
	}

	// Inequality operator for iterators
	template < typename C_Type, bool B_LHS, typename T_LHS, bool B_RHS, typename T_RHS >
	bool operator !=(const Iterator<C_Type, B_LHS, T_LHS>& lhs_, const Iterator<C_Type, B_RHS, T_RHS>& rhs_)
	{
		return !(lhs_ == rhs_);
	}

}



// Alias for the OutTree_ class template in the global namespace
template < typename T, typename TTypeTraits = nsOutTree::BasicTraits<T> >
using OutTree = nsOutTree::Container<T, TTypeTraits>;



