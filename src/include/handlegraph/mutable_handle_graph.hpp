#ifndef HANDLEGRAPH_MUTABLE_HANDLE_GRAPH_HPP_INCLUDED
#define HANDLEGRAPH_MUTABLE_HANDLE_GRAPH_HPP_INCLUDED

/** \file 
 * Defines the MutableHandleGraph interface for graphs that can be added to.
 */

#include "handlegraph/handle_graph.hpp"

#include <functional>
#include <vector>

namespace handlegraph {


/**
 * This is the interface for a handle graph that supports addition of new graph material.
 */
class MutableHandleGraph : virtual public HandleGraph {
public:
    /*
     * Note: All operations may invalidate path handles and occurrence handles.
     */
    
    /// Create a new node with the given sequence and return the handle.
    virtual handle_t create_handle(const std::string& sequence) = 0;

    /// Create a new node with the given id and sequence, then return the handle.
    virtual handle_t create_handle(const std::string& sequence, const nid_t& id) = 0;
    
    /// Create an edge connecting the given handles in the given order and orientations.
    /// Ignores existing edges.
    virtual void create_edge(const handle_t& left, const handle_t& right) = 0;
    
    /// Convenient wrapper for create_edge.
    inline void create_edge(const edge_t& edge) {
        create_edge(edge.first, edge.second);
    }
    
    /// Swap the nodes corresponding to the given handles, in the ordering used
    /// by for_each_handle when looping over the graph. Other handles to the
    /// nodes being swapped must not be invalidated. If a swap is made while
    /// for_each_handle is running, it affects the order of the handles
    /// traversed during the current traversal (so swapping an already seen
    /// handle to a later handle's position will make the seen handle be visited
    /// again and the later handle not be visited at all).
    virtual void swap_handles(const handle_t& a, const handle_t& b) = 0;

    /// Reorder the graph's internal structure to match that given.
    /// Optionally compact the id space of the graph to match the ordering, from 1->|ordering|.
    void apply_ordering(const std::vector<handle_t>& order, bool compact_ids = false) = 0;
    
    /// Alter the node that the given handle corresponds to so the orientation
    /// indicated by the handle becomes the node's local forward orientation.
    /// Rewrites all edges pointing to the node and the node's sequence to
    /// reflect this. Invalidates all handles to the node (including the one
    /// passed). Returns a new, valid handle to the node in its new forward
    /// orientation. Note that it is possible for the node's ID to change.
    /// Does not update any stored paths. May change the ordering of the underlying
    /// graph.
    virtual handle_t apply_orientation(const handle_t& handle) = 0;
    
    /// Split a handle's underlying node at the given offsets in the handle's
    /// orientation. Returns all of the handles to the parts. Other handles to
    /// the node being split may be invalidated. The split pieces stay in the
    /// same local forward orientation as the original node, but the returned
    /// handles come in the order and orientation appropriate for the handle
    /// passed in.
    /// Updates stored paths.
    virtual std::vector<handle_t> divide_handle(const handle_t& handle, const std::vector<size_t>& offsets) = 0;
    
    /// Specialization of divide_handle for a single division point
    inline std::pair<handle_t, handle_t> divide_handle(const handle_t& handle, size_t offset) {
        auto parts = divide_handle(handle, std::vector<size_t>{offset});
        return std::make_pair(parts.front(), parts.back());
    }
};

}

#endif
