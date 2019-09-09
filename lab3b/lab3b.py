#!/usr/bin/python

# NAME: John Tran
# EMAIL: johntran627@gmail.com
# UID: 005183495

from collections import defaultdict
import sys
import csv

class SuperBlock:
        def __init__(self, num_blocks, num_inodes, block_size, inode_size, first_inode):
		self.num_blocks = int(num_blocks)               
		self.num_inodes = int(num_inodes)               
		self.block_size = int(block_size)               
		self.inode_size = int(inode_size)                 
		self.first_inode = int(first_inode) 

class Group:
	def __init__(self, num_blocks, num_inodes, first_inode):           
		self.num_blocks = int(num_blocks)                              
		self.num_inodes = int(num_inodes)                          
		self.first_inode = int(first_inode)  

class BFree:
	def __init__(self, index):
		self.index = int(index)

class IFree:
	def __init__(self, index):
		self.index = int(index)

class INode:
	def __init__(self, inode_num, link_count, block_num):
		self.inode_num = int(inode_num)   
		self.link_count = int(link_count)                              
		self.direct_block = block_num[:12] 
		self.indirect_block = block_num[12:15]

class DirEnt:
	def __init__(self, parent_inode, file_inode, name):
		self.parent_inode = int(parent_inode) 
		self.file_inode = int(file_inode) 
		self.name = name                 

class Indirect:
	def __init__(self, inode_num, level, indirect_block, block_num):
		self.inode_num = int(inode_num)        
		self.level = int(level)                
		self.indirect_block = int(indirect_block) 
		self.block_num = int(block_num)          

class FileSystemImageLoader:
	def __init__(self, filename):
		self.status        = 0
		self.filename      = filename
		self.super_block   = None
		self.group         = None
		self.bfree_list    = []
		self.ifree_list    = []
		self.inode_list    = []
		self.dirent_list   = []
		self.indirect_list = []

	def load(self):
		try:
			with open(self.filename) as csv_file:
                                csv_reader = csv.reader(csv_file, delimiter=',')

				for row in csv_reader:
					if   row[0] == 'SUPERBLOCK': 
                                                self.super_block = SuperBlock(row[1], row[2], row[3], row[4], row[7])
					elif row[0] == 'GROUP':      
                                                self.group = Group(row[2], row[3], row[8])
					elif row[0] == 'BFREE':      
                                                self.bfree_list += [BFree(row[1])]
					elif row[0] == 'IFREE':      
                                                self.ifree_list += [IFree(row[1])]
					elif row[0] == 'INODE':      
                                                self.inode_list += [INode(row[1], row[6], list(map(int, row[12:])))]
					elif row[0] == 'DIRENT':     
                                                self.dirent_list += [DirEnt(row[1], row[3], row[6])]
					elif row[0] == 'INDIRECT':   
                                                self.indirect_list += [Indirect(row[1], row[2], row[4], row[5])]

		except IOError:
                        sys.stderr.write("Error opening the file.\n")
                        self.status = 1
		except IndexError:
                        sys.stderr.write("Invalid file format.\n")
                        self.status = 1
		except:
                        sys.stderr.write("Unexpected error.\n")
                        self.status = 1
                        

class BlockReference:
	def __init__(self, block_level, block_num, inode_num, block_offset):
		self.block_level  = block_level
		self.block_num = block_num
		self.inode_num = inode_num
		self.block_offset = block_offset

	def get_block_level_string(self):
		level_str = None
		if   (self.block_level == 0): level_str = "BLOCK"
		elif (self.block_level == 1): level_str = "INDIRECT BLOCK"
		elif (self.block_level == 2): level_str = "DOUBLE INDIRECT BLOCK"
		elif (self.block_level == 3): level_str = "TRIPLE INDIRECT BLOCK"
		return level_str

class FileSystemImageHandler:
	def __init__(self, loader):
		self.status = 0
		self.loader = loader

	def analyze(self):
		self.block_handler()
		self.inode_handler()
		self.directory_handler()

	def block_handler(self):
		first_non_reserved = self.loader.group.first_inode + self.loader.super_block.inode_size * self.loader.group.num_inodes / self.loader.super_block.block_size                 
		
		referenced_blocks = defaultdict(list)

		for inode in self.loader.inode_list:
			for i in range(len(inode.direct_block)):
				self.direct_block_handler(inode.direct_block[i], inode.inode_num, i, referenced_blocks, first_non_reserved)

			for i in range(len(inode.indirect_block)):
				self.indirect_block_handler(inode.indirect_block[i], inode.inode_num, i + 1, referenced_blocks, first_non_reserved)
				
		for indirect in self.loader.indirect_list:
			self.indirect_block_handler(indirect.block_num, indirect.inode_num, indirect.level, referenced_blocks, first_non_reserved)
                

                free_block_numbers = list(map(lambda bfree: bfree.index, self.loader.bfree_list))
                                
		for block_num in range(first_non_reserved, self.loader.super_block.num_blocks):
                        if block_num not in referenced_blocks and block_num not in free_block_numbers:
                                print("UNREFERENCED BLOCK {}".format(block_num))
                                self.status = 2
                        if block_num in referenced_blocks and block_num in free_block_numbers:
                                print("ALLOCATED BLOCK {} ON FREELIST".format(block_num))
                                self.status = 2
                
                        
                        block_reference_list = []
                        if block_num in referenced_blocks:
                                block_reference_list = referenced_blocks[block_num]
                        if len(block_reference_list) == 1: block_reference_list = []
                        
                        if block_reference_list:
                                for block_reference in block_reference_list:
                                        print("DUPLICATE {} {} IN INODE {} AT OFFSET {}".format(block_reference.get_block_level_string(), block_reference.block_num, block_reference.inode_num, block_reference.block_offset))
                                        self.status = 2
                                

	def direct_block_handler(self, direct_block, inode_num, offset, referenced_blocks, first_non_reserved):
		if direct_block == 0: return

		if direct_block < 0 or direct_block >= self.loader.super_block.num_blocks:
                        print("INVALID BLOCK {} IN INODE {} AT OFFSET {}".format(direct_block, inode_num, offset))
                        self.status = 2
		elif first_non_reserved > direct_block and direct_block > 0:         
                        print("RESERVED BLOCK {} IN INODE {} AT OFFSET {}".format(direct_block, inode_num, offset))
                        self.status = 2
		else:
			referenced_blocks[direct_block].append(BlockReference(0, direct_block, inode_num, offset))

	def indirect_block_handler(self, indirect_block, inode_num, level, referenced_blocks, first_non_reserved):
		if indirect_block == 0: return

		offset = None
		if   (level == 1): offset = 12
		elif (level == 2): offset = 12 + 256
		elif (level == 3): offset = 12 + (1 + 256) * 256
		
		if indirect_block < 0 or indirect_block >= self.loader.super_block.num_blocks:                
                        print("INVALID {} BLOCK {} IN INODE {} AT OFFSET {}".format(self.get_level_string(level), indirect_block, inode_num, offset))
                        self.status = 2
		elif first_non_reserved > indirect_block and indirect_block > 0: 
                        print("RESERVED {} BLOCK {} IN INODE {} AT OFFSET {}".format(self.get_level_string(level), indirect_block, inode_num, offset))
                        self.status = 2
		else:
			referenced_blocks[indirect_block].append(BlockReference(level, indirect_block, inode_num, offset))


	def inode_handler(self):
		free_inode_numbers = list(map(lambda ifree: ifree.index, self.loader.ifree_list))
		allocated_inode_numbers = list(map(lambda inode: inode.inode_num, self.loader.inode_list))
		
		for inode_num in allocated_inode_numbers:
			if inode_num in free_inode_numbers:
                                print("ALLOCATED INODE {} ON FREELIST".format(inode_num))
                                self.status = 2
                                	
		for inode_num in range(self.loader.super_block.first_inode, self.loader.super_block.num_inodes + 1):
			if inode_num not in allocated_inode_numbers and inode_num not in free_inode_numbers: 
                                print("UNALLOCATED INODE {} NOT ON FREELIST".format(inode_num))
                                self.status = 2

                                
	def directory_handler(self):	
		allocated_inode_numbers = list(map(lambda inode: inode.inode_num, self.loader.inode_list))
		link_count_dict = defaultdict(int)
		parent_dict = {}
		
		for dirent in self.loader.dirent_list: 
			if dirent.file_inode < 1 or dirent.file_inode > self.loader.super_block.num_inodes:
				print("DIRECTORY INODE {} NAME {} INVALID INODE {}".format(dirent.parent_inode, dirent.name, dirent.file_inode))
                                self.status = 2                                
			elif dirent.file_inode not in allocated_inode_numbers:
				print("DIRECTORY INODE {} NAME {} UNALLOCATED INODE {}".format(dirent.parent_inode, dirent.name, dirent.file_inode))
                                self.status = 2
			else:
				link_count_dict[dirent.file_inode] += 1
				if dirent.file_inode not in parent_dict: parent_dict[dirent.file_inode] = dirent.parent_inode
				

		for inode in self.loader.inode_list:
			if inode.link_count != link_count_dict[inode.inode_num]:
				print("INODE {} HAS {} LINKS BUT LINKCOUNT IS {}".format(inode.inode_num, link_count_dict[inode.inode_num], inode.link_count))
                                self.status = 2
                                

		for dirent in self.loader.dirent_list:
			if dirent.name == "'.'" and dirent.parent_inode != dirent.file_inode:
				print("DIRECTORY INODE {} NAME {} LINK TO INODE {} SHOULD BE {}".format(dirent.parent_inode, dirent.name, dirent.file_inode, dirent.parent_inode))
                                self.status = 2                                
			elif dirent.name == "'..'" and parent_dict[dirent.parent_inode] != dirent.file_inode:
                                print("DIRECTORY INODE {} NAME {} LINK TO INODE {} SHOULD BE {}".format(dirent.parent_inode, dirent.name, dirent.file_inode, parent_dict[dirent.parent_inode]))
                                self.status = 2
                                

	def get_level_string(self, level):
		level_str = None
		if   (level == 1): level_str = "INDIRECT"
		elif (level == 2): level_str = "DOUBLE INDIRECT"
		elif (level == 3): level_str = "TRIPLE INDIRECT"
		return level_str


def main():
	if len(sys.argv) != 2: 
                sys.stderr.write("Invalid number of arguments.\n")
                sys.exit(1)

	loader = FileSystemImageLoader(sys.argv[1])
	loader.load()

	if (loader.status == 1): sys.exit(1)

	file_handler = FileSystemImageHandler(loader)
	file_handler.analyze()

	sys.exit(file_handler.status)

if __name__ == '__main__':
	main()





