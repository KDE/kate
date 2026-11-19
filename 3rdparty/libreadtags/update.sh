# SPDX-FileCopyrightText: None
# SPDX-License-Identifier: CC0-1.0

# get sources
curl https://raw.githubusercontent.com/universal-ctags/libreadtags/refs/heads/master/readtags.c > readtags.c
curl https://raw.githubusercontent.com/universal-ctags/libreadtags/refs/heads/master/readtags.h > readtags.h

# avoid unused parameters
sed 's/return isPseudoTagLine (file->name.buffer);/(void) unused; return isPseudoTagLine (file->name.buffer);/g' -i readtags.c
sed 's/return (nameComparison (file) == 0);/(void) unused; return (nameComparison (file) == 0);/g' -i readtags.c
