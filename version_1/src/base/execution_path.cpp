#include "execution_path.h"

#include "../util/stuffs.h"
#include <functional>
#include <algorithm>

//typedef std::function<conditions_t ()> lazy_func_cond_t;


/**
 * @brief two_sub_conds_are_identical
 */
static auto two_sub_conds_are_identical (const condition_t& sub_cond_a,
                                         const condition_t& sub_cond_b) -> bool
{
  return (std::equal(std::get<1>(sub_cond_a).begin(),
                     std::get<1>(sub_cond_a).end(), std::get<1>(sub_cond_b).begin()) &&
          two_vmaps_are_identical(std::get<0>(sub_cond_a), std::get<0>(sub_cond_b)));
}


/**
 * @brief map_exists_in_maps
 */
static auto map_exists_in_maps (const addrint_value_map_t& tested_map,
                                const addrint_value_maps_t& maps) -> bool
{
  auto is_identical = std::bind(two_maps_are_identical, tested_map, std::placeholders::_1);
  return (std::find_if(maps.begin(), maps.end(), is_identical) != maps.end());
}


/**
 * @brief remove duplicated map inside a vector of maps
 */
static auto remove_duplicated (const addrint_value_maps_t& input_maps) -> addrint_value_maps_t
{
  auto no_change = [&input_maps]() -> addrint_value_maps_t { return input_maps; };
  auto remove_from = [&input_maps]() -> addrint_value_maps_t
  {
    addrint_value_maps_t result_maps;
    std::for_each(input_maps.begin(), input_maps.end(),
                  [&](addrint_value_maps_t::const_reference examined_map)
    {
      auto is_identical = std::bind(two_maps_are_identical, examined_map,
                                                 std::placeholders::_1);
      if (std::find_if(result_maps.begin(), result_maps.end(), is_identical) == result_maps.end())
      {
        result_maps.push_back(examined_map);
      }
    });
    return result_maps;
  };

  return input_maps.size() > 1 ? remove_from() : no_change();
}


/**
 * @brief verifying if two maps a and b are of the same type, i.e. the sets of addresses of a and
 * of b are the same
 */
//static auto are_of_the_same_type (const addrint_value_map_t& map_a,
//                                  const addrint_value_map_t& map_b) -> bool
//{
//  return ((map_a.size() == map_b.size()) &&
//          std::equal(map_a.begin(), map_a.end(), map_b.begin(),
//                     [](addrint_value_map_t::const_reference a_elem,
//                        addrint_value_map_t::const_reference b_elem)
//                     {
//                       // verify if the address in a is equal to the one in b
//                         return (a_elem.first == b_elem.first);
//                     }));
//};


/**
 * @brief verify if two maps a and b are "strictly" isomorphic, i.e. there is a isomorphism
 * (function) f making
 *                                    map_a
 *                             A --------------> V
 *                             |                 |
 *                           f |                 | f = 1
 *                             V                 V
 *                             B --------------> V
 *                                    map_b
 * commutative, and f follows the address ordering in A and B
 */
//static auto are_isomorphic (const addrint_value_map_t& map_a,
//                            const addrint_value_map_t& map_b) -> bool
//{
//  return ((map_a.size() == map_b.size()) &&
//          std::equal(map_a.begin(), map_a.end(), map_b.begin(),
//                     [](addrint_value_map_t::const_reference a_elem,
//                        addrint_value_map_t::const_reference b_elem)
//                     {
//                       return (a_elem.second == b_elem.second);
//                     }));
//}


/**
 * @brief verify if hom(A,V) and hom(B,V) are isomorphic, i.e. there is an isomorphism (functor) F
 * making
 *                                    map_a
 *                             A --------------> V
 *                             |                 |
 *                           F |                 | F = 1
 *                             V                 V
 *                             B --------------> V
 *                                    map_b
 * commutative
 */
//static auto are_isomorphic (const addrint_value_maps_t& maps_a,
//                            const addrint_value_maps_t& maps_b) -> bool
//{
//  return ((maps_a.size() == maps_b.size()) &&
//          std::all_of(maps_a.begin(), maps_a.end(),
//                      [&](addrint_value_maps_t::const_reference maps_a_elem) -> bool
//  {
//    return std::any_of(maps_b.begin(), maps_b.end(),
//                       [&](addrint_value_maps_t::const_reference maps_b_elem) -> bool
//    {
//      return are_isomorphic(maps_a_elem, maps_b_elem);
//    });
//  }));
//}


/**
 * @brief calculate stabilized condition which can be considered as a cartesian product
 *                             A x ... x B x ...
 * of disjoint sub-conditions
 */
static auto stabilize (const conditions_t& raw_cond) -> conditions_t
{
  // lambda verifying if two maps a and b have an intersection
  auto have_intersection = [](const addrint_value_map_t& map_a,
                              const addrint_value_map_t& map_b) -> bool
  {
    // verify if there exists some element of a which is also an element of b
    return std::any_of(map_a.begin(), map_a.end(),
                       [&map_a,&map_b](addrint_value_map_t::value_type map_a_elem) -> bool
    {
      return (map_b.find(map_a_elem.first) != map_b.end());
    });
  };

  // calculating join (least upper bound or cartesian product) of two maps a and b: the
  // priority of the map b is higher on one of a, i.e. the map b is the condition of some (or
  // several branches) that are placed lower than one of a
  auto join_maps = [](const addrint_value_maps_t& maps_a,
                      const addrint_value_maps_t& maps_b) -> addrint_value_maps_t
  {
    auto get_addrs = [](const addrint_value_map_t addrs_vals) -> addrints_t
    {
      /*addrints_t*/auto addrs = addrints_t();
      std::for_each(addrs_vals.begin(), addrs_vals.end(),
                    [&addrs](addrint_value_map_t::const_reference addr_val)
      {
        addrs.push_back(addr_val.first);
      });

      return addrs;
    };

    auto intersection = [](const addrints_t& addrs_a, const addrints_t& addrs_b) -> addrints_t
    {
      /*addrints_t*/auto addrs_aib = addrints_t();
      std::set_intersection(addrs_a.begin(), addrs_a.end(),
                            addrs_b.begin(), addrs_b.end(), std::back_inserter(addrs_aib));
      return addrs_aib;
    };

    auto difference = [](const addrints_t& addrs_a, const addrints_t& addrs_b) -> addrints_t
    {
      /*addrints_t*/ auto addrs_adb = addrints_t();
      std::set_difference(addrs_a.begin(), addrs_a.end(),
                          addrs_b.begin(), addrs_b.end(), std::back_inserter(addrs_adb));
      return addrs_adb;
    };

    auto maps_projection = [](
        const addrint_value_maps_t& maps, const addrints_t& addrs) -> addrint_value_maps_t
    {
      addrint_value_maps_t projected_maps;

      std::for_each(maps.begin(), maps.end(), [&](addrint_value_map_t addr_val_map)
      {
        addrint_value_map_t projected_map;
        std::for_each(addrs.begin(), addrs.end(),
                      [&projected_map, &addr_val_map](addrints_t::const_reference addr)
        {
          projected_map[addr] = addr_val_map[addr];
        });

        if (!map_exists_in_maps(projected_map, projected_maps))
          projected_maps.push_back(projected_map);
      });
      return projected_maps;
    };


    /*addrints_t*/auto addrs_a = get_addrs(maps_a.front());
    /*addrints_t*/auto addrs_b = get_addrs(maps_b.front());

    /*addrints_t*/auto addrs_aib = intersection(addrs_a, addrs_b);
    /*addrints_t*/auto addrs_adb = difference(addrs_a, addrs_b);
    /*addrints_t*/auto addrs_bda = difference(addrs_b, addrs_a);

//    tfm::format(std::cerr, "%d %d %d\n", addrs_aib.size(), addrs_adb.size(), addrs_bda.size());

    auto maps_adb = maps_projection(maps_a, addrs_adb);
    auto maps_bda = maps_projection(maps_b, addrs_bda);
    // because the priority of b is higher than a, so use the values of b
    auto maps_aib = maps_projection(maps_b, addrs_aib);

    // calculate the cartesian product of 3 maps
    /*addrint_value_maps_t*/auto maps_ab = addrint_value_maps_t();
    std::for_each(maps_adb.begin(), maps_adb.end(),
                  [&](addrint_value_maps_t::const_reference map_adb)
    {
      std::for_each(maps_aib.begin(), maps_aib.end(),
                    [&](addrint_value_maps_t::const_reference map_aib)
      {
        std::for_each(maps_bda.begin(), maps_bda.end(),
                      [&](addrint_value_maps_t::const_reference map_bda)
        {
          auto map_sum = map_adb;
          map_sum.insert(map_aib.begin(), map_aib.end());
          map_sum.insert(map_bda.begin(), map_bda.end());

          maps_ab.push_back(map_sum);
        });
      });
    });

//    tfm::format(std::cerr, "joined map size %d\n", maps_ab.size());

    return maps_ab;
  };

  // calculating join of two list of cfi
  auto join_cfis = [](const ptr_cond_direct_inss_t& cfis_a,
                      const ptr_cond_direct_inss_t& cfis_b) -> ptr_cond_direct_inss_t
  {
//    tfm::format(std::cerr, "add a new cfi into a sub-cond\n"); std:exit(1);
    auto joined_list = cfis_a;
    std::for_each(cfis_b.begin(), cfis_b.end(), [&](ptr_cond_direct_inss_t::const_reference cfi_b)
    {
      // verify if a element of cfis_b exists also in cfis_a
      if (std::find(cfis_a.begin(), cfis_a.end(), cfi_b) == cfis_a.end())
      {
        // does not exist, then add it
        joined_list.push_back(cfi_b);
      }
    });

    return joined_list;
  };

  auto erase_sub_cond_from_cond = [](const condition_t& sub_cond, conditions_t& cond) -> void
  {
    auto cond_iter = std::find_if(cond.begin(), cond.end(),
                                  std::bind(two_sub_conds_are_identical,
                                            sub_cond, std::placeholders::_1));
    if (cond_iter != cond.end()) cond.erase(cond_iter);
    else tfm::format(std::cerr, "sub condition not found\n");

    return;
  };

  auto replace_sub_cond_in_cond = [](const condition_t& replaced_cond,
      const condition_t& replacing_cond, conditions_t& cond) -> void
  {
    auto replaced_cond_iter =
        std::find_if(cond.begin(), cond.end(),
                     std::bind(two_sub_conds_are_identical, replaced_cond, std::placeholders::_1));
    if (replaced_cond_iter != cond.end()) *replaced_cond_iter = replacing_cond;
  };


  // the following loop makes the condition converge on a stabilized state: it modifies the
  // condition by merging intersected sub-conditions until no such intersection is found
  auto examined_cond = raw_cond;
  auto intersection_exists = false;

  do
  {
    intersection_exists = false;

//    tfm::format(std::cerr, "examined condition size %d\n", examined_cond.size());

    // for each pair of sub-conditions
    for (auto sub_cond_a_iter = examined_cond.begin(); sub_cond_a_iter != examined_cond.end();
         ++sub_cond_a_iter)
    {
//      tfm::format(std::cerr, "a size %d\n", sub_cond_a->first.size());
      for (auto sub_cond_b_iter = std::next(sub_cond_a_iter); sub_cond_b_iter != examined_cond.end();
           ++sub_cond_b_iter)
      {
//        tfm::format(std::cerr, "b size %d\n", sub_cond_b->first.size());
        // verify if they have intersection
        if (have_intersection(*(std::get<0>(*sub_cond_a_iter).begin()),
                              *(std::get<0>(*sub_cond_b_iter).begin())))
        {
          // yes
          intersection_exists = true;

          // then join their maps
          auto joined_maps = join_maps(std::get<0>(*sub_cond_a_iter), std::get<0>(*sub_cond_b_iter));
          // and join their cfi
          auto joined_cfis = join_cfis(std::get<1>(*sub_cond_a_iter), std::get<1>(*sub_cond_b_iter));

          // erase sub-condition a and b from the path condition, NOTE the side-effect: the input
          // condition examining_cond will be modified
          auto cp_a = *sub_cond_a_iter; auto cp_b = *sub_cond_b_iter;
          erase_sub_cond_from_cond(cp_a, examined_cond);
          replace_sub_cond_in_cond(cp_b, std::make_pair(joined_maps, joined_cfis), examined_cond);

          // because the curr_cond has been modified, both iterators cond_elem_a and cond_elem_b
          // have been made invalid, breakout to restart the verification
          break;
        }
      }
      if (intersection_exists) break;
    }
  }
  while (intersection_exists);

  return examined_cond;
}


///**
// * @brief verify if the condition is recursive
// */
//auto is_recursive (const conditions_t& stabilized_cond) -> bool
//{
//  return ((stabilized_cond.size() >= 2) &&
//          (are_isomorphic(stabilized_cond.crbegin()->first,
//                          std::next(stabilized_cond.crbegin())->first)));
//}


/**
 * @brief calculate the non-recursive order of the condition
 */
//auto order (const conditions_t& stabilized_cond) -> int
//{
//  conditions_t::const_reverse_iterator examining_cond_iter = stabilized_cond.crbegin();
//  std::all_of(stabilized_cond.crbegin(), stabilized_cond.crend(),
//              [&](conditions_t::const_reference cond_elem) -> bool
//  {
//    return ((stabilized_cond.crend() - examining_cond_iter >= 2) &&
//            are_isomorphic(examining_cond_iter->first, (++examining_cond_iter)->first));
//  });

//  return stabilized_cond.crend() - examining_cond_iter;
//}


/**
 * @brief reconstruct path condition as a cartesian product A x ... x B x ...
 */
static auto calculate_from (const order_ins_map_t& current_path,
                            const path_code_t& current_path_code) -> conditions_t
{
  auto raw_condition = conditions_t();
  auto current_code_order = 0;

//  tfm::format(std::cerr, "----\ncurrent path length %d with code size %d: %s\n", current_path.size(),
//              current_path_code.size(), path_code_to_string(current_path_code));

  std::for_each(current_path.begin(),
                current_path.end(), [&](order_ins_map_t::const_reference order_ins)
  {
    // verify if the current instruction is a cfi
    if (order_ins.second->is_cond_direct_cf)
    {
      // yes, then downcast it as a CFI
//      auto current_cfi = std::static_pointer_cast<cond_direct_instruction>(order_ins.second);
      auto current_cfi = look_for_saved_instance(
            std::static_pointer_cast<cond_direct_instruction>(order_ins.second), current_path_code);
      if (current_cfi)
      {
        // verify if this CFI is resolved
        if (current_cfi->is_resolved)
        {
          // look into the path code to know which condition should be added
//          if (!current_path_code[current_code_order])
//            raw_condition.push_back(
//                  std::make_pair(remove_duplicated(current_cfi->first_input_projections),
//                                                   ptr_cond_direct_inss_t(1, current_cfi)));
//          else raw_condition.push_back(
//                std::make_pair(remove_duplicated(current_cfi->second_input_projections),
//                                                      ptr_cond_direct_inss_t(1, current_cfi)));
          !current_path_code[current_code_order] ?
                raw_condition.push_back(
                  std::make_pair(remove_duplicated(current_cfi->first_input_projections),
                                 ptr_cond_direct_inss_t(1, current_cfi))) :
                raw_condition.push_back(
                  std::make_pair(remove_duplicated(current_cfi->second_input_projections),
                                 ptr_cond_direct_inss_t(1, current_cfi)));

//          tfm::format(std::cerr, "cfi is resolved\n");
        }
        else
        {
//          tfm::format(std::cerr, "cfi at %s is not resolved\n",
//                      addrint_to_hexstring(current_cfi->address));
        }
        current_code_order++;
      }
    }
  });

//  tfm::format(std::cerr, "stabilizing raw condition with size %d\n", raw_condition.size());
//  std::for_each(
//        raw_condition.begin(), raw_condition.end(), [](conditions_t::const_reference sub_cond)
//  {
//    tfm::format(std::cerr, "%d ", std::get<0>(sub_cond).size());
//  });
//  tfm::format(std::cerr, "\n");

  return stabilize(raw_condition);
}


/**
 * @brief constructor
 */
execution_path::execution_path (const order_ins_map_t& path, const path_code_t& path_code)
{
  this->content = path;
  this->code = path_code;
}


/**
 * @brief calculate path condition
 */
auto execution_path::calculate_condition () -> void
{
  this->condition = calculate_from(this->content, this->code);
//  this->condition_is_recursive = is_recursive(this->condition);
//  this->condition_order = order(this->condition);

#if !defined(NDEBUG)
//  std::for_each(this->condition.begin(), this->condition.end(),
//                [](conditions_t::const_reference sub_cond)
//  {
//    tfm::format(std::cerr, "| ");
//    std::for_each(sub_cond.first.begin()->begin(), sub_cond.first.begin()->end(),
//                  [](addrint_value_map_t::const_reference addr_val)
//    {
//      tfm::format(std::cerr, "%s ", addrint_to_hexstring(addr_val.first));
//    });
//    tfm::format(std::cerr, "|\n");
//  });

//  std::string output_filename = "path_" + path_code_to_string(this->code) +
//      "_" + process_id_str + ".log";
//  save_path_condition(this->condition, output_filename);
#endif
  return;
}


/**
 * @brief predict a lazy condition
 */
//auto execution_path::lazy_condition (int n) -> conditions_t
//{
//  conditions_t lazy_cond(this->condition.begin(),
//                         std::next(this->condition.begin(), std::min(n, this->condition_order)));
//  if (this->condition_is_recursive)
//  {
//    while (this->condition_order < n--) lazy_cond.push_back(this->condition.back());
//  }

//  return lazy_cond;
//}


/**
 * @brief calculate conditions of all explored paths
 */
auto calculate_exec_path_conditions(ptr_exec_paths_t& exec_paths) -> void
{
  std::for_each(exec_paths.begin(), exec_paths.end(), [](ptr_exec_paths_t::value_type path)
  {
    path->calculate_condition();
  });
  return;
}


#if !defined(NDEBUG)
/**
 * @brief show_path_condition
 */
//auto show_path_condition(const ptr_exec_paths_t& exec_paths) -> void
//{
//  tfm::format(std::cout, "path conditions\n");
//  auto show_cond = [&](ptr_exec_paths_t::const_reference exp_path, int i) -> void
//  {
//    tfm::format(std::cout, "| ");
//    std::for_each(exp_path->condition.at(i).first.begin()->begin(),
//                  exp_path->condition.at(i).first.begin()->end(),
//                  [&](addrint_value_map_t::const_reference cond_elem)
//    {
//      tfm::format(std::cout, "%s ", addrint_to_hexstring(cond_elem.first));
//    });
//    return;
//  };

//  std::for_each(exec_paths.begin(), exec_paths.end(), [&](ptr_exec_paths_t::const_reference exp_path)
//  {
//    auto i = 0;
//    for (; i < exp_path->condition_order; ++i) show_cond(exp_path, i);
//    if (exp_path->condition_is_recursive) tfm::format(std::cout, "|*\n");
//    else tfm::format(std::cout, "|\n");
//  });
//  return;
//}


//auto show_path_condition(const ptr_exec_path_t& exp_path) -> void
//{
//  tfm::format(std::cout, "| ");
//  std::for_each(exp_path->condition.at(i).first.begin()->begin(),
//                exp_path->condition.at(i).first.begin()->end(),
//                [&](addrint_value_map_t::const_reference cond_elem)
//  {
//    tfm::format(std::cout, "%s ", addrint_to_hexstring(cond_elem.first));
//  });

//  return;
//}
#endif
