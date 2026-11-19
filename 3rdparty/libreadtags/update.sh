# SPDX-FileCopyrightText: None
# SPDX-License-Identifier: CC0-1.0

# get sources
curl https://raw.githubusercontent.com/universal-ctags/libreadtags/refs/heads/master/readtags.c > readtags.c
curl https://raw.githubusercontent.com/universal-ctags/libreadtags/refs/heads/master/readtags.h > readtags.h

# avoid unused parameters
sed s/unused//g -i readtags.c
