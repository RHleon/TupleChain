#ifndef PU_DISPATCH_H
#define PU_DISPATCH_H

#include <cstdio>
#include <multibit-trie.hpp>
#include <trie-overlap.hpp>
#include <trie-merge.hpp>
#include <sail.hpp>
#include <sail-m.hpp>
#include <tail-trie.hpp>
#include <bitmap-tree.hpp>
#include <fixed-stride-trie.hpp>
#include <d24.hpp>
#include <art.hpp>
#include <dxr.hpp>
#include <poptrie.hpp>
#include <sail4.hpp>
#include <sail_4.hpp>
#include <sail_6.hpp>
#include <poptrie6.hpp>

#define PU_DISPATCH(args, process)					\
    if (args.algorithmName == "mbt") {					\
	switch(args.type) {						\
	case pp::DataType::IPV4:					\
	    process(new pp::MultibitTrie<pp::ipv4_addr>(args.treeHeight, args.mbtMode)); \
	    break;							\
	default: printf("!!!! invalid data type for IP Lookup\n");	\
	}								\
    }									\
    else if (args.algorithmName == "bmt") {				\
	switch(args.type) {						\
	case pp::DataType::IPV4:					\
	    process(new pp::BitmapTree<pp::ipv4_addr>(8, 6));		\
	    break;							\
	default: printf("!!!! invalid data type for IP Lookup\n");	\
	}								\
    }									\
    else if (args.algorithmName == "fst") {				\
	switch(args.type) {						\
	case pp::DataType::IPV4:					\
	    process(new pp::FixedStrideTrie<pp::ipv4_addr>());		\
	    break;							\
	default: printf("!!!! invalid data type for IP Lookup\n");	\
	}								\
    }									\
    else if (args.algorithmName == "sail") {				\
	switch(args.type) {						\
	case pp::DataType::IPV4:					\
	    switch(args.nhBytes) {					\
	    case 1: process(new pp::Sail4<uint8_t>()); break;		\
	    default: process(new pp::Sail4<uint32_t>()); break;		\
	    }								\
	    break;							\
	default: printf("!!!! invalid data type for IP Lookup\n");	\
	}								\
    }									\
    else if (args.algorithmName == "d24") {				\
	switch(args.type) {						\
	case pp::DataType::IPV4:					\
	    switch(args.nhBytes) {					\
	    case 1: process(new pp::Dir24<uint8_t>()); break;		\
	    default: process(new pp::Dir24<uint32_t>()); break;		\
	    }								\
	    break;							\
	default: printf("!!!! invalid data type for IP Lookup\n");	\
	}								\
    }									\
    else if (args.algorithmName == "art") {				\
	ART_ASSIST_DISPATCH(args, process)				\
    }									\
    else if (args.algorithmName == "art6") {				\
        args.customizedParameters = "6";					\
        ART_ASSIST_DISPATCH(args, process)				\
    }									\
    else if (args.algorithmName == "d248") {				\
	switch(args.type) {						\
	case pp::DataType::IPV4:					\
	    switch(args.nhBytes) {					\
	    case 1: process(new pp::Dir248<uint8_t>()); break;		\
	    default: process(new pp::Dir248<uint32_t>()); break;	\
	    }								\
	    break;							\
	default: printf("!!!! invalid data type for IP Lookup\n");	\
	}								\
    }									\
    else if (args.algorithmName == "vto") {				\
	if (args.treeHeight == 0) {					\
	    switch(args.type) {						\
	    case pp::DataType::VIP4:					\
		process(new pp::TrieOverlap<pp::ipv4_addr>(args.nInstances)); \
		break;							\
	    default: printf("!!!! invalid data type for IP Lookup\n");	\
	    }								\
	} else {							\
	    switch(args.type) {						\
	    case pp::DataType::VIP4:					\
		process(new pp::TrieOverlapE<pp::ipv4_addr>(args.nInstances, args.treeHeight)); \
		break;							\
	    default: printf("!!!! invalid data type for IP Lookup\n");	\
	    }								\
	}								\
    }									\
    else if (args.algorithmName == "vtm") {				\
	switch(args.type) {						\
	case pp::DataType::VIP4:					\
	    process(new pp::TrieMerge<pp::ipv4_addr>(args.nInstances)); \
	    break;							\
	default: printf("!!!! invalid data type for IP Lookup\n");	\
	}								\
    }									\
    else if (args.algorithmName == "vsm") {				\
	switch(args.type) {						\
	case pp::DataType::VIP4:					\
	    process(new pp::SailM4(args.nInstances));	\
	    break;							\
	default: printf("!!!! invalid data type for IP Lookup\n");	\
	}								\
    }									\
    else if (args.algorithmName == "vtt") {				\
	switch(args.type) {						\
	case pp::DataType::VIP4:					\
	    process(new pp::TailTrie<pp::ipv4_addr>(args.nInstances, args.reservedMemory)); \
	    break;							\
	default: printf("!!!! invalid data type for IP Lookup\n");	\
	}								\
    }									\
	else if (args.algorithmName == "dxr") {					\
	switch(args.type) {						\
	case pp::DataType::IPV4:					\
	    switch(args.nhBytes) {					\
	    case 1: process(new pp::DXR<uint8_t>()); break;		\
	    default: process(new pp::DXR<uint32_t>()); break;		\
	    }								\
	    break;							\
	default: printf("!!!! invalid data type for IP Lookup\n");	\
	}								\
    }										\
	else if (args.algorithmName == "poptrie") {					\
	switch(args.type) {						\
	case pp::DataType::IPV4:					\
	    switch(args.nhBytes) {					\
	    case 1: process(new pp::POPTRIE<uint8_t>()); break;		\
	    default: process(new pp::POPTRIE<uint32_t>()); break;	\
	    }								\
	    break;							\
	default: printf("!!!! invalid data type for IP Lookup\n");	\
	}								\
    }										\
	else if (args.algorithmName == "sail4") {					\
	switch(args.type) {						\
	case pp::DataType::IPV4:					\
	    process(new pp::SAIL4<uint32_t>()); break;		\
	    break;							\
	default: printf("!!!! invalid data type for IP Lookup\n");	\
	}								\
    }										\
	else if (args.algorithmName == "sail_4") {					\
	    switch(args.type) {						\
	    case pp::DataType::IPV4:					\
		switch(args.nhBytes) {					\
		case 1: process(new pp::SAIL_4<uint8_t>()); break;	\
		default: process(new pp::SAIL_4<uint32_t>()); break;	\
		}							\
		break;							\
	    default: printf("!!!! invalid data type for IP Lookup\n");	\
	    }								\
	}								\
	else if (args.algorithmName == "sail_6") {			\
	    switch(args.type) {						\
	    case pp::DataType::IPV6:					\
		switch(args.nhBytes) {					\
		case 1: process(new pp::SAIL_6<uint8_t>()); break;	\
		default: process(new pp::SAIL_6<uint32_t>()); break;	\
		}							\
		break;							\
	    default: printf("!!!! invalid data type for IP Lookup\n");	\
	    }								\
	}								\
	else if (args.algorithmName == "poptrie6") {					\
	switch(args.type) {						\
	case pp::DataType::IPV6:					\
	    switch(args.nhBytes) {					\
	    case 1: process(new pp::POPTRIE6<uint8_t>()); break;	\
	    default: process(new pp::POPTRIE6<uint32_t>()); break;	\
	    }								\
		break;							\
	default: printf("!!!! invalid data type for IP Lookup\n");	\
	}								\
    }										\
    else {								\
	printf("!!!!! unsupported algorithms\n");			\
	exit(1);							\
    }

#endif
