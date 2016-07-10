Tries to pick a nearby analysis for tagged analyses which are not available
from lt-proc. Currently just picks a match when the major (1st) tag matches.
When there are multiple, picks the one with most matching tags.

Usage:

    ./sync-corpus TAGGED_CORPUS UNTAGGED_CORPUS > RETAGGED_CORPUS
