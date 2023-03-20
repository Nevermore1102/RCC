#pragma once

#include <libdevcore/CommonData.h>
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>
#include <libdevcore/FixedHash.h>
#include <libdevcore/Address.h>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <queue>
#include <chrono>
#include <string>

using namespace std;

namespace dev {
namespace eth {
class Transaction;
}

namespace consensus {
    extern int hiera_shard_number;
    extern int internal_groupId;
}
}

namespace dev {
namespace plugin {

class ExecuteVM;

#define PLUGIN_LOG(LEVEL) LOG(LEVEL) << LOG_BADGE("PLUGIN") << LOG_BADGE("PLUGIN")

// 用于计算最近公共祖先
class Hierarchical_Shard
{
    public:
        using Ptr = std::shared_ptr<Hierarchical_Shard>;

        Hierarchical_Shard(int shardid){ id = shardid; };
        Hierarchical_Shard::Ptr getLCA(Hierarchical_Shard::Ptr shard1, Hierarchical_Shard::Ptr shard2) {
			// Create a set to store ancestors of node
			std::set<Ptr> ancestors;
			// Traverse the complete tree and store ancestors
			// in the set
			while (shard1) {
				ancestors.insert(shard1);
				shard1 = shard1->preshard;
			}

			// Check if shard2 or any of its ancestors is
			// in the set
			while (shard2) {
				if (ancestors.find(shard2) != ancestors.end())
					return shard2;
				shard2 = shard2->preshard;
			}

			// If we reach here, return NULL
			return nullptr;
		}

    public:
        int id = 0;
        Hierarchical_Shard::Ptr preshard = nullptr;
};

// // 层级化结构信息
// class Hierarchical_Structure
// {
//     public:
//         using Ptr = std::shared_ptr<Hierarchical_Structure>;
//         Hierarchical_Structure()
//         {
//             shards = make_shared<vector<Hierarchical_Shard::Ptr>>();

//             if(dev::consensus::hiera_shard_number == 9) {
//                 auto shard1 = make_shared<Hierarchical_Shard>(1);
//                 auto shard2 = make_shared<Hierarchical_Shard>(2);
//                 auto shard3 = make_shared<Hierarchical_Shard>(3);
//                 auto shard4 = make_shared<Hierarchical_Shard>(4);
//                 auto shard5 = make_shared<Hierarchical_Shard>(5);
//                 auto shard6 = make_shared<Hierarchical_Shard>(6);
//                 auto shard7 = make_shared<Hierarchical_Shard>(7);
//                 auto shard8 = make_shared<Hierarchical_Shard>(8);
//                 auto shard9 = make_shared<Hierarchical_Shard>(9);

//                 shard1->preshard = shard4;
//                 shard2->preshard = shard4;
//                 shard3->preshard = shard4;
//                 shard5->preshard = shard8;
//                 shard6->preshard = shard8;
//                 shard7->preshard = shard8;
//                 shard4->preshard = shard9;
//                 shard8->preshard = shard9;

//                 shards->push_back(shard1);
//                 shards->push_back(shard2);
//                 shards->push_back(shard3);
//                 shards->push_back(shard4);
//                 shards->push_back(shard5);
//                 shards->push_back(shard6);
//                 shards->push_back(shard7);
//                 shards->push_back(shard8);
//                 shards->push_back(shard9);
//             }
//             else if(dev::consensus::hiera_shard_number == 3) {
//                 auto shard1 = make_shared<Hierarchical_Shard>(1);
//                 auto shard2 = make_shared<Hierarchical_Shard>(2);
//                 auto shard3 = make_shared<Hierarchical_Shard>(3);

//                 shard1->preshard = shard3;
//                 shard2->preshard = shard3;

//                 shards->push_back(shard1);
//                 shards->push_back(shard2);
//                 shards->push_back(shard3);
//             }
//         }

//     public:
//         shared_ptr<vector<Hierarchical_Shard::Ptr>> shards;
// };

// extern int global_internal_groupId;
extern shared_ptr<ExecuteVM> executiveContext;
// extern string nodeIdHex;

    }  // namespace plugin
}  // namespace dev