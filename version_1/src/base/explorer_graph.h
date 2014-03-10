#ifndef EXPLORER_GRAPH_H
#define EXPLORER_GRAPH_H

#include "cond_direct_instruction.h"

class explorer_graph;
typedef std::shared_ptr<explorer_graph>   ptr_explorer_graph_t;
typedef std::vector<bool>                 path_code_t;

class explorer_graph
{
public:
  static ptr_explorer_graph_t instance(); // allow only a single instance of explorer graph
  void add_vertex(ADDRINT ins_addr);
  void add_edge(ADDRINT ins_a_addr, ADDRINT ins_b_addr, const path_code_t& edge_path_code,
                const addrint_value_map_t& edge_addrs_values = addrint_value_map_t());
  void save_to_file(std::string filename);

private:
  explorer_graph();
};


#endif // EXPLORER_GRAPH_H
