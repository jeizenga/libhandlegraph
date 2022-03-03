/** \file path_metadata.cpp
 * Implement PathMetadata interface's default implementation.
 */

#include "handlegraph/path_metadata.hpp"

namespace handlegraph {

// These are all our no-value placeholders.
const std::string PathMetadata::NO_SAMPLE_NAME = "";
const std::string PathMetadata::NO_LOCUS_NAME = "";
const int64_t PathMetadata::NO_HAPLOTYPE = -1;
const int64_t PathMetadata::NO_PHASE_BLOCK = -1;
const int64_t PathMetadata::NO_END_POSITION = -1;
const std::pair<int64_t, int64_t> PathMetadata::NO_SUBRANGE{-1, PathMetadata::NO_END_POSITION};

// And these are the constants for parsing path names out into metadata
const char PathMetadata::SEPARATOR = '#';
const char PathMetadata::RANGE_START_SEPARATOR = '[';
const char PathMetadata::RANGE_END_SEPARATOR = '-';
const char PathMetadata::RANGE_TERMINATOR = ']';

// Format examples:
// GRCh38#chrM (a reference)
// CHM13#chr12 (another reference)
// CHM13#chr12[300-400] (part of a reference)
// NA19239#1#chr1 (a diploid reference)
// NA29239#1#chr1#0 (a haplotype)
// 1[100] (part of a generic path)
// We don't support extraneous [] in name components in the structured format, or in names with ranges.
// TODO: escape them?

// So we match a regax for:
// One separator-free name component
// Up to 3 other optional separator-free name components, led by separators tacked on by non-capturing groups. Last one must be a number.
// Possibly a bracket-bounded non-capturing group at the end
// Which has a number, and possibly a dash-led non-capturing group with a number.
// Match number:                         1           2             3             4           5        6
const std::regex PathMetadata::FORMAT(R"(([^[#]*)(?:#([^[#]*))?(?:#([^[#]*))?(?:#(\d+))?(?:\[(\d+)(?:-(\d+))\])?)");
const size_t PathMetadata::ASSEMBLY_OR_NAME_MATCH = 1;
const size_t PathMetadata::LOCUS_MATCH_WITHOUT_HAPLOTYPE = 2;
const size_t PathMetadata::HAPLOTYPE_MATCH = 2;
const size_t PathMetadata::LOCUS_MATCH_WITH_HAPLOTYPE = 3;
const size_t PathMetadata::PHASE_BLOCK_MATCH = 4;
const size_t PathMetadata::RANGE_START_MATCH = 5;
const size_t PathMetadata::RANGE_END_MATCH = 6;


PathMetadata::Sense PathMetadata::get_sense(const path_handle_t& handle) const {
    // Get the name.
    std::string path_name = get_path_name(handle);
    // Match the regex
    std::smatch result;
    if (std::regex_match(path_name, result, FORMAT)) {
        // It's something we know.
        if (result[PHASE_BLOCK_MATCH].matched) {
            // It's a haplotype because it has a phase block
            return SENSE_HAPLOTYPE;
        } else {
            // It's a reference
            return SENSE_REFERENCE;
        }
    } else {
        // We can't parse this at all.
        return SENSE_GENERIC;
    }
}



std::string PathMetadata::get_sample_name(const path_handle_t& handle) const {
    // Get the name.
    std::string path_name = get_path_name(handle);
    // Match the regex
    std::smatch result;
    if (std::regex_match(path_name, result, FORMAT)) {
        if (result[LOCUS_MATCH_WITH_HAPLOTYPE].matched || result[LOCUS_MATCH_WITHOUT_HAPLOTYPE].matched) {
            // There's a locus later, so the first thing doesn't have to be locus, so it can be sample.
            return result[ASSEMBLY_OR_NAME_MATCH].str();
        } else {
            // There's nothing but the locus and maybe a range.
            return NO_SAMPLE_NAME;
        }
    } else {
        // No sample name.
        return NO_SAMPLE_NAME;
    }
}


std::string PathMetadata::get_locus_name(const path_handle_t& handle) const {
    // Get the name.
    std::string path_name = get_path_name(handle);
    // Match the regex
    std::smatch result;
    if (std::regex_match(path_name, result, FORMAT)) {
        if (result[LOCUS_MATCH_WITH_HAPLOTYPE].matched) {
            // There's a phase and a locus
            return result[LOCUS_MATCH_WITH_HAPLOTYPE].str();
        } else if (result[LOCUS_MATCH_WITHOUT_HAPLOTYPE].matched) {
            // There's a locus but no phase
            return result[LOCUS_MATCH_WITHOUT_HAPLOTYPE].str();
        } else {
            // There's nothing but the locus and maybe a range.
            return result[ASSEMBLY_OR_NAME_MATCH].str();
        }
    } else {
        // Just the whole thing should come out here.
        return path_name;
    }
}


int64_t PathMetadata::get_haplotype(const path_handle_t& handle) const {
    // Get the name.
    std::string path_name = get_path_name(handle);
    // Match the regex
    std::smatch result;
    if (std::regex_match(path_name, result, FORMAT)) {
        if (result[LOCUS_MATCH_WITH_HAPLOTYPE].matched) {
            // There's a haplotype.
            // We just assume it's actually a number.
            return std::stoll(result[HAPLOTYPE_MATCH].str());
        } else {
            // No haplotype is stored
            return NO_HAPLOTYPE;
        }
    } else {
        // We can't parse this at all.
        return NO_HAPLOTYPE;
    }
}


int64_t PathMetadata::get_phase_block(const path_handle_t& handle) const {
    // Get the name.
    std::string path_name = get_path_name(handle);
    // Match the regex
    std::smatch result;
    if (std::regex_match(path_name, result, FORMAT)) {
        if (result[PHASE_BLOCK_MATCH].matched) {
            // There's a phase block.
            // We just assume it's actually a number.
            return std::stoll(result[PHASE_BLOCK_MATCH].str());
        } else {
            // No phase block is stored
            return NO_PHASE_BLOCK;
        }
    } else {
        // We can't parse this at all.
        return NO_PHASE_BLOCK;
    }
}

std::pair<int64_t, int64_t> PathMetadata::get_subrange(const path_handle_t& handle) const {
    auto to_return = NO_SUBRANGE;
    
    // Get the name.
    std::string path_name = get_path_name(handle);
    // Match the regex
    std::smatch result;
    if (std::regex_match(path_name, result, FORMAT)) {
        if (result[RANGE_START_MATCH].matched) {
            // There is a range start, so pasre it
            to_return.first = std::stoll(result[RANGE_START_MATCH].str());
            if (result[RANGE_END_MATCH].matched) {
                // There is also an end, so parse that too
                to_return.second = std::stoll(result[RANGE_END_MATCH].str());
            }
        }
    }
    
    return to_return;
}

}

