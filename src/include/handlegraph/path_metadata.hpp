#ifndef HANDLEGRAPH_PATH_METADATA_HPP_INCLUDED
#define HANDLEGRAPH_PATH_METADATA_HPP_INCLUDED

/** \file 
 * Defines the metadata API for paths
 */

#include "handlegraph/handle_graph.hpp"

#include <utility>
#include <string>
#include <regex>

namespace handlegraph {

/**
 * This is the interface for embedded path and haplotype thread metadata.
 *
 * Comes with a default implementation of this interface, based on
 * a get_path_name() and special path name formatting.
 *
 * Our model is that paths come in different "senses":
 *
 * - SENSE_GENERIC: a generic named path. Has a "locus" name.
 *
 * - SENSE_REFERENCE: a part of a reference assembly. Has a "sample" name, a
 *   "locus" name, and a haplotype number.
 *
 * - SENSE_HAPLOTYPE: a haplotype from a particular individual. Has a "sample"
 *   name, a "locus" name, a haplotype number, and a phase block identifier.
 *
 * Paths of all sneses can represent subpaths, with bounds.
 *
 * Depending on sense, a path might have:
 *
 * - Sample: sample or assembly name.
 *
 * - Locus: contig, scaffold, or gene name path either represents in its
 *   assembly or is an allele of in its sample.
 *
 * - Haplotype number: number identifying which haplotype of a locus is being
 *   represented. GFA uses a convention where the presence of a haplotype 0
 *   implies that only one haplotype is present.
 *
 * - Phase block identifier: Distinguishes fragments of a haplotype that are
 *   phased but not necessarily part of a single self-consistent scaffold (often
 *   due to self-contradictory VCF information). Must be unique within a sample,
 *   locus, and haplotype. May be a number or a start coordinate.
 *
 * - Bounds, for when a path as stored gives only a sub-range of a conceptually
 *   longer path. Multiple items can be stored with identical metadata in the
 *   other fields if their bounds are non-overlapping.
 * TODO: Interaction with phase block in GBWT???
 */
class PathMetadata {
public:
    
    virtual ~PathMetadata() = default;
    
    /// Each path always has just one sense.
    enum Sense {
        SENSE_GENERIC,
        SENSE_REFERENCE,
        SENSE_HAPLOTYPE
    };
    
    ////////////////////////////////////////////////////////////////////////////
    // Path metadata interface that has a default implementation
    ////////////////////////////////////////////////////////////////////////////
    
    /// What is the given path meant to be representing?
    virtual Sense get_sense(const path_handle_t& handle) const;
    
    /// Get the name of the sample or assembly asociated with the
    /// path-or-thread, or NO_SAMPLE_NAME if it does not belong to one.
    virtual std::string get_sample_name(const path_handle_t& handle) const;
    static const std::string NO_SAMPLE_NAME;
    
    /// Get the name of the contig or gene asociated with the path-or-thread,
    /// or NO_LOCUS_NAME if it does not belong to one.
    virtual std::string get_locus_name(const path_handle_t& handle) const;
    static const std::string NO_LOCUS_NAME;
    
    /// Get the haplotype number (0 or 1, for diploid) of the path-or-thread,
    /// or NO_HAPLOTYPE if it does not belong to one.
    virtual int64_t get_haplotype(const path_handle_t& handle) const;
    static const int64_t NO_HAPLOTYPE;
    
    /// Get the phase block number (contiguously phased region of a sample,
    /// contig, and haplotype) of the path-or-thread, or NO_PHASE_BLOCK if it
    /// does not belong to one.
    virtual int64_t get_phase_block(const path_handle_t& handle) const;
    static const int64_t NO_PHASE_BLOCK;
    
    /// Get the bounds of the path-or-thread that are actually represented
    /// here. Should be NO_SUBRANGE if the entirety is represented here, and
    /// 0-based inclusive start and exclusive end positions of the stored 
    /// region on the full path-or-thread if a subregion is stored.
    ///
    /// If no end position is stored, NO_END_POSITION may be returned for the
    /// end position.
    virtual std::pair<int64_t, int64_t> get_subrange(const path_handle_t& handle) const;
    static const std::pair<int64_t, int64_t> NO_SUBRANGE;
    static const int64_t NO_END_POSITION;
    
    ////////////////////////////////////////////////////////////////////////////
    // Stock interface that uses backing virtual methods
    ////////////////////////////////////////////////////////////////////////////
    
    /// Loop through all the paths with the given sense. Returns false and
    /// stops if the iteratee returns false.
    template<typename Iteratee>
    bool for_each_path_of_sense(const Sense& sense, const Iteratee& iteratee) const;
    
    /// Loop through all steps on the given handle for paths with the given
    /// sense. Returns false and stops if the iteratee returns false.
    /// TODO: Take the opportunity to make steps track orientation better?
    template<typename Iteratee>
    bool for_each_step_of_sense(const handle_t& visited, const Sense& sense, const Iteratee& iteratee) const;
    
protected:
    
    ////////////////////////////////////////////////////////////////////////////
    // Backing protected virtual methods that have a default implementation
    ////////////////////////////////////////////////////////////////////////////
    
    /// Loop through all the paths with the given sense. Returns false and
    /// stops if the iteratee returns false.
    virtual bool for_each_path_of_sense_impl(const Sense& sense, const std::function<bool(const path_handle_t&)>& iteratee) const;
    
    /// Loop through all steps on the given handle for paths with the given
    /// sense. Returns false and stops if the iteratee returns false.
    virtual bool for_each_step_of_sense_impl(const handle_t& visited, const Sense& sense, const std::function<bool(const step_handle_t&)>& iteratee) const;
    
    ////////////////////////////////////////////////////////////////////////////
    // Backing methods that need to be implemented for default implementation
    ////////////////////////////////////////////////////////////////////////////
    
    /// Look up the name of a path from a handle to it
    virtual std::string get_path_name(const path_handle_t& path_handle) const = 0;
    
private:
    
    ////////////////////////////////////////////////////////////////////////////
    // Internal machinery for path name mini-format
    ////////////////////////////////////////////////////////////////////////////
    
    /// Separator used to separate path name components
    static const char SEPARATOR;
    // Ranges are set off with some additional characters.
    static const char RANGE_START_SEPARATOR;
    static const char RANGE_END_SEPARATOR;
    static const char RANGE_TERMINATOR;
    
    // And we use these for parsing
    static const std::regex FORMAT;
    static const size_t ASSEMBLY_OR_NAME_MATCH;
    static const size_t LOCUS_MATCH_WITHOUT_HAPLOTYPE;
    static const size_t HAPLOTYPE_MATCH;
    static const size_t LOCUS_MATCH_WITH_HAPLOTYPE;
    static const size_t PHASE_BLOCK_MATCH;
    static const size_t RANGE_START_MATCH;
    static const size_t RANGE_END_MATCH;
};

////////////////////////////////////////////////////////////////////////////
// Template Implementations
////////////////////////////////////////////////////////////////////////////

template<typename Iteratee>
bool PathMetadata::for_each_path_of_sense(const Sense& sense, const Iteratee& iteratee) const {
    return for_each_path_of_sense_impl(sense, BoolReturningWrapper<Iteratee>::wrap(iteratee));
}

template<typename Iteratee>
bool PathMetadata::for_each_step_of_sense(const handle_t& visited, const Sense& sense, const Iteratee& iteratee) const {
    return for_each_step_of_sense_impl(visited, sense, BoolReturningWrapper<Iteratee>::wrap(iteratee));
}

}
#endif
