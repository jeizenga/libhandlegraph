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

// Format examples:
// GRCh38#chrM (a reference)
// CHM13#chr12 (another reference)
// CHM13#chr12[300-400] (part of a reference)
// NA19239#1#chr1 (a diploid reference)
// NA29239#1#chr1#0 (a haplotype)
// 1[100] (part of a generic path)
// We don't support extraneous [] in name components in the structured format, or in names with ranges.
// TODO: escape them?

// So we match a regex for:
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

// And these are the constants for composing path names from metadata
const char PathMetadata::SEPARATOR = '#';
const char PathMetadata::RANGE_START_SEPARATOR = '[';
const char PathMetadata::RANGE_END_SEPARATOR = '-';
const char PathMetadata::RANGE_TERMINATOR = ']';


PathMetadata::Sense PathMetadata::get_sense(const path_handle_t& handle) const {
    return PathMetadata::parse_sense(get_path_name(handle));
}

std::string PathMetadata::get_sample_name(const path_handle_t& handle) const {
    return PathMetadata::parse_sample_name(get_path_name(handle));
}

std::string PathMetadata::get_locus_name(const path_handle_t& handle) const {
    return PathMetadata::parse_locus_name(get_path_name(handle));
}

int64_t PathMetadata::get_haplotype(const path_handle_t& handle) const {
    return PathMetadata::parse_haplotype(get_path_name(handle));
}

int64_t PathMetadata::get_phase_block(const path_handle_t& handle) const {
    return PathMetadata::parse_phase_block(get_path_name(handle));
}

std::pair<int64_t, int64_t> PathMetadata::get_subrange(const path_handle_t& handle) const {
    return PathMetadata::parse_subrange(get_path_name(handle));
}

PathMetadata::Sense PathMetadata::parse_sense(const std::string& path_name) {
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



std::string PathMetadata::parse_sample_name(const std::string& path_name) {
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


std::string PathMetadata::parse_locus_name(const std::string& path_name) {
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


int64_t PathMetadata::parse_haplotype(const std::string& path_name) {
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


int64_t PathMetadata::parse_phase_block(const std::string& path_name) {
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

std::pair<int64_t, int64_t> PathMetadata::parse_subrange(const std::string& path_name) {
    auto to_return = NO_SUBRANGE;
    
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

void PathMetadata::parse_path_name(const std::string& path_name,
                                   PathMetadata::Sense& sense,
                                   std::string& sample,
                                   std::string& locus,
                                   int64_t& haplotype,
                                   int64_t& phase_block,
                                   std::pair<int64_t, int64_t>& subrange) {

    std::smatch result;
    auto matched = std::regex_match(path_name, result, FORMAT);
    
    // Parse out each piece.
    // TODO: can we unify this with the other places we parse out from the
    // regex? With yet a third set of functions?
    if (matched) {
        if (result[PHASE_BLOCK_MATCH].matched) {
            sense = SENSE_HAPLOTYPE;
        } else {
            sense = SENSE_REFERENCE;
        }
        
        if (result[LOCUS_MATCH_WITH_HAPLOTYPE].matched) {
            // There's a phase and a locus and a sample
            sample = result[ASSEMBLY_OR_NAME_MATCH].str();
            locus = result[LOCUS_MATCH_WITH_HAPLOTYPE].str();
            haplotype = std::stoll(result[HAPLOTYPE_MATCH].str());
        } else if (result[LOCUS_MATCH_WITHOUT_HAPLOTYPE].matched) {
            // There's a locus but no phase, and a sample
            sample = result[ASSEMBLY_OR_NAME_MATCH].str();
            locus = result[LOCUS_MATCH_WITHOUT_HAPLOTYPE].str();
            haplotype = NO_HAPLOTYPE;
        } else {
            // There's nothing but the locus and maybe a range.
            sample = NO_SAMPLE_NAME;
            locus = result[ASSEMBLY_OR_NAME_MATCH].str();
            haplotype = NO_HAPLOTYPE;
        }
        
        if (result[PHASE_BLOCK_MATCH].matched) {
            // There's a phase block.
            // We just assume it's actually a number.
            phase_block = std::stoll(result[PHASE_BLOCK_MATCH].str());
        } else {
            // No phase block is stored
            phase_block = NO_PHASE_BLOCK;
        }
        
        if (result[RANGE_START_MATCH].matched) {
            // There is a range start, so pasre it
            subrange.first = std::stoll(result[RANGE_START_MATCH].str());
            if (result[RANGE_END_MATCH].matched) {
                // There is also an end, so parse that too
                subrange.second = std::stoll(result[RANGE_END_MATCH].str());
            } else {
                subrange.second = NO_END_POSITION;
            }
        } else {
            subrange = NO_SUBRANGE;
        }
    } else {
        // Just a generic path where the locus is all of it.
        sense = SENSE_GENERIC;
        sample = NO_SAMPLE_NAME;
        locus = path_name;
        haplotype = NO_HAPLOTYPE;
        phase_block = NO_PHASE_BLOCK;
        subrange = NO_SUBRANGE;
    }
}

std::string PathMetadata::create_path_name(const PathMetadata::Sense& sense,
                                           const std::string& sample,
                                           const std::string& locus,
                                           const int64_t& haplotype,
                                           const int64_t& phase_block,
                                           const std::pair<int64_t, int64_t>& subrange) {
    
    std::stringstream name_builder;
    
    if (sample != NO_SAMPLE_NAME) {
        if (sense == SENSE_GENERIC) {
            throw std::runtime_error("Generic path must have a sample");
        }
        name_builder << sample << SEPARATOR;
    }
    if (locus != NO_LOCUS_NAME) {
        name_builder << locus;
    } else {
        if (sense == SENSE_GENERIC) {
            throw std::runtime_error("Generic path must have a locus/name");
        } else if (sense == SENSE_REFERENCE) {
            throw std::runtime_error("Referecne path must have a locus");
        } else if (sense == SENSE_HAPLOTYPE) {
            throw std::runtime_error("Haplotype path must have a locus");
        }
    }
    if (haplotype != NO_HAPLOTYPE) {
        if (sense == SENSE_GENERIC) {
            throw std::runtime_error("Generic path must have a haplotype number");
        }
        name_builder << SEPARATOR << haplotype;
    } else {
        if (sense == SENSE_HAPLOTYPE) {
            throw std::runtime_error("Haplotype path must have a haplotype number");
        }
    }
    if (phase_block != NO_PHASE_BLOCK) {
        if (sense == SENSE_GENERIC) {
            throw std::runtime_error("Generic path must have a phase block");
        } else if (sense == SENSE_REFERENCE) {
            throw std::runtime_error("Reference path must have a phase block");
        }
        name_builder << SEPARATOR << phase_block;
    } else {
        if (sense == SENSE_HAPLOTYPE) {
            throw std::runtime_error("Haplotype path must have a phase block");
        }
    }
    if (subrange != NO_SUBRANGE) {
        // Everything can have a subrange.
        name_builder << RANGE_START_SEPARATOR << subrange.first;
        if (subrange.second != NO_END_POSITION) {
            name_builder << RANGE_END_SEPARATOR << subrange.second;
        }
        name_builder << RANGE_TERMINATOR;
    }
    
    return name_builder.str();
}

bool PathMetadata::for_each_path_matching_impl(const std::unordered_set<PathMetadata::Sense>* senses,
                                               const std::unordered_set<std::string>* samples,
                                               const std::unordered_set<std::string>* loci,
                                               const std::function<bool(const path_handle_t&)>& iteratee) const {
    return for_each_path_handle_impl([&](const path_handle_t& handle) {
        if (senses && !senses->count(get_sense(handle))) {
            // Wrong sense
            return true;
        }
        if (samples && !samples->count(get_sample_name(handle))) {
            // Wrong sample
            return true;
        }
        if (loci && !loci->count(get_locus_name(handle))) {
            // Wrong sample
            return true;
        }
        // And emit any matching handles
        return iteratee(handle);
    });
}
    
bool PathMetadata::for_each_step_of_sense_impl(const handle_t& visited, const Sense& sense, const std::function<bool(const step_handle_t&)>& iteratee) const {
    return for_each_step_on_handle_impl(visited, [&](const step_handle_t& handle) {
        if (get_sense(get_path_handle_of_step(handle)) != sense) {
            // Skip this non-matching path's step
            return true;
        }
        // And emit any steps on matching paths
        return iteratee(handle);
    });
}

}

