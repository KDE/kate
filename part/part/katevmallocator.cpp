/*
    This file is part of the KDE libraries

    Copyright (C) 1999 Waldo Bastian (bastian@kde.org)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/
//----------------------------------------------------------------------------
//
// Virtual Memory Allocator

// TODO: Add large file support.
// TODO: Error reporting. (e.g. disk-full)

#include <unistd.h>
#include <sys/mman.h>

#include <qintdict.h>
#include <qmap.h>

#include <ktempfile.h>
#include <kdebug.h>

#include "katevmallocator.h"


#define KVM_ALIGN 4095

struct KVMAllocator::Block
{
   off_t start;
   size_t length; // Requested length
   size_t size; // Actual size
   void *mmap;
};


class KVMAllocatorPrivate
{
public:
   KTempFile *tempfile;
   off_t max_length;
   QMap<off_t, KVMAllocator::Block> used_blocks;
   QMap<off_t, KVMAllocator::Block> free_blocks;
};

/**
 * Create a KVMAllocator
 */
KVMAllocator::KVMAllocator()
{
   d = new KVMAllocatorPrivate;
   d->tempfile = 0;
   d->max_length = 0;
}

/**
 * Destruct the KVMAllocator and release all memory.
 */
KVMAllocator::~KVMAllocator()
{
   delete d->tempfile;
   delete d;
}

/**
 * Allocate a virtual memory block.
 * @param _size Size in bytes of the memory block.
 */
KVMAllocator::Block *
KVMAllocator::allocate(size_t _size)
{
   if (!d->tempfile)
   {
      d->tempfile = new KTempFile("/tmp/", "vmdata");
      d->tempfile->unlink();
   }
   // Search in free list
   QMap<off_t,KVMAllocator::Block>::iterator it;
   it = d->free_blocks.begin();
   while (it != d->free_blocks.end())
   {
      if (it.data().size > _size)
      {
         Block &free_block = it.data();
         Block block;
         kdDebug(13020)<<"VM alloc: using block from free list "<<(long)free_block.start<<" size ="<<(long)free_block.size<<" request = "<<_size<< endl;
         block.start = free_block.start;
         block.length = _size;
         block.size = (_size + KVM_ALIGN) & ~KVM_ALIGN;
         block.mmap = 0;
         free_block.size -= block.size;
         free_block.start += block.size;
         if (!free_block.size)
            d->free_blocks.remove(it);
         it = d->used_blocks.replace(block.start, block);
         return &(it.data());
      }
      ++it;
   } 


   // Create new block
   Block block;
   block.start = d->max_length;
   block.length = _size;
   block.size = (_size + KVM_ALIGN) & ~KVM_ALIGN;
   block.mmap = 0;
   kdDebug(13020)<<"VM alloc: using new block "<<(long)block.start<<" size ="<<(long)block.size<<" request = "<<_size<< endl;
   it = d->used_blocks.replace(block.start, block);
   d->max_length += block.size;
   return &(it.data());
}

/**
 * Free a virtual memory block
 */
void
KVMAllocator::free(Block *block_p)
{
   Block block = *block_p;
   if (block.mmap)
   {
      kdDebug(13020)<<"VM free: Block "<<(long)block.start<<" is still mmapped!"<<endl;
      return;
   }
   QMap<off_t,KVMAllocator::Block>::iterator it;
   it = d->used_blocks.find(block.start);
   if (it == d->used_blocks.end())
   {
      kdDebug(13020)<<"VM free: Block "<<(long)block.start<<" is not allocated."<<endl;
      return;
   }
   d->used_blocks.remove(it);
   it = d->free_blocks.replace(block.start, block);
   QMap<off_t,KVMAllocator::Block>::iterator before = it;
   --before;
   if (before != d->free_blocks.end())
   {
      Block &block_before = before.data();
      if ((block_before.start + block_before.size) == block.start)
      {
         // Merge blocks.
         kdDebug(13020) << "VM merging: Block "<< (long)block_before.start<<
                           " with "<< (long)block.start<< " (before)" << endl;
         block.size += block_before.size;
         block.start = block_before.start;
         it.data() = block;
         d->free_blocks.remove(before);
      }
   }
   
   QMap<off_t,KVMAllocator::Block>::iterator after = it;
   ++after;
   if (after != d->free_blocks.end())
   {
      Block &block_after = after.data();
      if ((block.start + block.size) == block_after.start)
      {
         // Merge blocks.
         kdDebug(13020) << "VM merging: Block "<< (long)block.start<<
                           " with "<< (long)block_after.start<< " (after)" << endl;
         block.size += block_after.size;
         it.data() = block;
         d->free_blocks.remove(after);
      }
   }
}

/**
 * Copy data from a virtual memory block to normal memory
 */
void
KVMAllocator::copy(void *dest, Block *src, int _offset, size_t length)
{
   //kdDebug(13020)<<"VM read: seek "<<(long)src->start<<" +"<<_offset<<":"<<length<<endl;
   lseek(d->tempfile->handle(), src->start+_offset, SEEK_SET);
   if (length == 0)
      length = src->length - _offset;
   int to_go = length;
   int done = 0;
   char *buf = (char *) dest;
   while(done < to_go)
   {
      int n = read(d->tempfile->handle(), buf+done, to_go);
      if (n <= 0) return; // End of data or error
      done += n;
      to_go -= n;
   }
   // Done.
}

/**
 * Copy data from normal memory to a virtual memory block
 */
void
KVMAllocator::copy(Block *dest, void *src, int _offset, size_t length)
{
   //kdDebug(13020)<<"VM write: seek "<<(long)dest->start<<" +"<<_offset<< ":" << length << endl;
   lseek(d->tempfile->handle(), dest->start+_offset, SEEK_SET);
   if (length == 0)
      length = dest->length - _offset;
   int to_go = length;
   int done = 0;
   char *buf = (char *) src;
   while(done < to_go)
   {
      int n = write(d->tempfile->handle(), buf+done, to_go);
      if (n <= 0) return; // End of data or error
      done += n;
      to_go -= n;
   }
   // Done.
}

/**
 * Map a virtual memory block in memory
 */
void *
KVMAllocator::map(Block *block)
{
   if (block->mmap)
      return block->mmap;

   void *result = mmap(0, block->length, PROT_READ| PROT_WRITE,
                       MAP_SHARED, d->tempfile->handle(), 0);
   block->mmap = result;
   return block->mmap;
}

/**
 * Unmap a virtual memory block
 */
void
KVMAllocator::unmap(Block *block)
{
   // The following cast is necassery for Solaris.
   // (touch it and die). --Waba
   munmap((char *)block->mmap, block->length);
   block->mmap = 0;
}
