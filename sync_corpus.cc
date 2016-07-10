#include <apertium/stream.h>
#include <apertium/shell_utils.h>
#include <apertium/streamed_type.h>
#include <apertium/string_utils.h>
#include <apertium/exception.h>
#include <iostream>

namespace Apertium {
using namespace ShellUtils;

bool findReplacements(
    Optional<Analysis> &ref_analysis, std::vector<Analysis> &src_analyses) {
  if (ref_analysis->TheMorphemes.size() == 1) {
    Morpheme &tagged_wordoid = ref_analysis->TheMorphemes.front();
    std::vector<Analysis>::const_iterator analyses_it;
    size_t best_intersection = 0;
    std::vector<Analysis>::const_iterator best_analysis;
    for (analyses_it = src_analyses.begin();
         analyses_it != src_analyses.end(); analyses_it++) {
      if (analyses_it->TheMorphemes.size() == 1) {
        const Morpheme &untagged_wordoid = analyses_it->TheMorphemes.front();
        std::vector<Tag> intersection;
        std::set_intersection(
          tagged_wordoid.TheTags.begin(), tagged_wordoid.TheTags.end(),
          untagged_wordoid.TheTags.begin(), untagged_wordoid.TheTags.end(),
          std::back_inserter(intersection));
        size_t intersection_size = intersection.size();
        if (tagged_wordoid.TheTags.front() == untagged_wordoid.TheTags.front() &&
            intersection_size > best_intersection) {
          best_intersection = intersection_size;
          best_analysis = analyses_it;
        }
      }
    }
    if (best_intersection >= 1) {
      std::wcerr << L"Replacement found: " << *best_analysis << L". (It shares "
        << best_intersection << L" tags.)\n";
      ref_analysis = *best_analysis;
      return true;
    }
  }
  return false;
}

struct CaseInsensitiveEq {
  const Analysis &ref_analysis;
  CaseInsensitiveEq(const Analysis &ref_analysis) : ref_analysis(ref_analysis) {};
  bool operator() (const Analysis &cand_analysis) {
    const std::vector<Morpheme> &ref_wordoids = ref_analysis.TheMorphemes;
    const std::vector<Morpheme> &cand_wordoids = cand_analysis.TheMorphemes;
    if (ref_wordoids.size() != cand_wordoids.size()) {
      return false;
    }
    for (size_t i = 0; i < ref_wordoids.size(); i++) {
      if (StringUtils::tolower(ref_wordoids[i].TheLemma) !=
          StringUtils::tolower(cand_wordoids[i].TheLemma)) {
        return false;
      }
      if (ref_wordoids[i].TheTags != cand_wordoids[i].TheTags) {
        return false;
      }
    }
    return true;
  }
};

void syncCorpus(int &argc, char **&argv) {
  std::locale::global(std::locale(""));
  expect_file_arguments(argc - 1, 2);

  basic_Tagger::Flags flags;

  std::wifstream tagged_corpus_stream;
  try_open_fstream("TAGGED_CORPUS", argv[1], tagged_corpus_stream);
  Stream tagged(flags, tagged_corpus_stream, argv[1]);

  std::wifstream untagged_corpus_stream;
  try_open_fstream("UNTAGGED_CORPUS", argv[2], untagged_corpus_stream);
  Stream untagged(flags, untagged_corpus_stream, argv[2]);

  size_t count_bad_unreplaced = 0;
  size_t count_bad_replaced = 0;

  flags.setShowSuperficial(true);

  while (1) {
    StreamedType tagged_token = tagged.get();
    StreamedType untagged_token = untagged.get();
    if (!tagged_token.TheLexicalUnit || !untagged_token.TheLexicalUnit) {
      if (tagged_token.TheLexicalUnit || untagged_token.TheLexicalUnit) {
        std::stringstream what_;
        what_ << "One stream has ended prematurely. "
              << "Please check if they are aligned.\n";
        throw Exception::UnalignedStreams(what_);
      }
      break;
    }
    std::wcout << tagged_token.TheString;
    Optional<Analysis> ref_analysis;

    std::vector<Analysis> &ref_analyses
        = tagged_token.TheLexicalUnit->TheAnalyses;
    std::vector<Analysis> &src_analyses
        = untagged_token.TheLexicalUnit->TheAnalyses;
    if (!ref_analyses.empty() && !src_analyses.empty()) {
      ref_analysis = ref_analyses.front();
      std::vector<Analysis>::const_iterator match_it
          = std::find(src_analyses.begin(), src_analyses.end(),
                      *ref_analysis);
      if (match_it == src_analyses.end()) {
        std::wcerr << "No exact match found for " << *ref_analysis
                   << L" on line " << tagged.TheLineNumber << ".\n";
        match_it = std::find_if(src_analyses.begin(), src_analyses.end(),
                                CaseInsensitiveEq(*ref_analysis));
        if (match_it == src_analyses.end()) {
          bool replacement_found = findReplacements(ref_analysis, src_analyses);
          if (replacement_found) {
            count_bad_replaced++;
          } else {
            std::wcerr << L"No replacement found. (leaving as is)\n";
            count_bad_unreplaced++;
          }
        } else {
          std::wcerr << L"...but it's a case insensitive match.\n";
        }
      }
    } else if (!ref_analyses.empty() && src_analyses.empty()) {
      std::wcerr << untagged_token.TheLexicalUnit->TheSurfaceForm
                 << L" on line " << tagged.TheLineNumber
                 << L" is tagged but has zero dictionary analyses "
                 << L"(leaving as is).\n";
    }

    Stream::outputLexicalUnit(*tagged_token.TheLexicalUnit, ref_analysis, std::wcout, flags);
  }

  std::wcerr << count_bad_replaced << L" symbols replaced.\n";
  std::wcerr << count_bad_unreplaced << L" problematic symbols left without replacement.\n";
}
}

int main(int argc, char **argv) {
  try {
    Apertium::syncCorpus(argc, argv);
  } catch (const Apertium::basic_ExceptionType &basic_ExceptionType_) {
    std::wcerr << argv[0] << ": " << basic_ExceptionType_.what() << std::endl;
    std::wcerr << "Usage " << argv[0] << " TAGGED_CORPUS UNTAGGED_CORPUS"
               << " > RETAGGED_CORPUS" << std::endl;
    exit(-2);
  } catch (...) {
    throw;
  }
}
