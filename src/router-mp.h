/***************************************************************************
 *  Project:    osmprob
 *  File:       router.h
 *  Language:   C++
 *
 *  osmprob is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  osmprob is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  osm-router.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Author:     Mark Padgham 
 *  E-Mail:     mark.padgham@email.com 
 *
 *  Description:    Current a skeleton Dijkstra mostly adapted from 
 *                  https://rosettacode.org/wiki/Dijkstra%27s_algorithm#C.2B.2B
 *
 *  Limitations:
 *
 *  Dependencies:       
 *
 *  Compiler Options:   -std=c++11 
 ***************************************************************************/


#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <limits> // for numeric_limits
#include <set>
#include <utility> // for pair
#include <algorithm>
#include <iterator>

#include <boost/numeric/ublas/matrix.hpp>

typedef int vertex_t;
typedef double weight_t;

const weight_t max_weight = std::numeric_limits <weight_t>::infinity();

struct neighbor {
    vertex_t target;
    weight_t weight;
    neighbor (vertex_t arg_target, weight_t arg_weight)
        : target (arg_target), weight (arg_weight) { }
};

typedef std::vector <std::vector <neighbor> > adjacency_list_t;
typedef boost::numeric::ublas::matrix <weight_t> weight_arr;

class Graph
{
    protected:
        unsigned _start_node, _end_node, _num_vertices;
        const std::vector <vertex_t> _idfrom, _idto;
        const std::vector <weight_t> _d;

    public:
        adjacency_list_t adjlist; // the graph data
        weight_arr cost_mat;

        Graph (std::vector <vertex_t> idfrom, std::vector <vertex_t> idto,
                std::vector <weight_t> d, unsigned start_node, unsigned end_node)
            : _idfrom (idfrom), _idto (idto), _d (d),
                _start_node (start_node), _end_node (end_node)
        {
            _num_vertices = fillGraph (); // fills adjlist with (idfrom, idto, d)
            make_cost_mat ();
        }
        ~Graph ()
        {
            for (int i=0; i<adjlist.size (); i++)
                adjlist [i].clear ();
            adjlist.clear ();
        }

        unsigned return_num_vertices() { return _num_vertices;   }
        unsigned return_start_node() { return _start_node;   }
        unsigned return_end_node() { return _end_node;   }
        std::vector <vertex_t> return_idfrom() { return _idfrom; }
        std::vector <vertex_t> return_idto() { return _idto; }
        std::vector <weight_t> return_d() { return _d; }

        unsigned fillGraph ();
        void Dijkstra (vertex_t source, 
                std::vector <weight_t> &min_distance,
                std::vector <vertex_t> &previous);
        std::vector <vertex_t> GetShortestPathTo (vertex_t vertex, 
                const std::vector <vertex_t> &previous);

        void make_cost_mat ();
};


/************************************************************************
 ************************************************************************
 **                                                                    **
 **                             FILLGRAPH                              **
 **                                                                    **
 ************************************************************************
 ************************************************************************/

unsigned Graph::fillGraph ()
{
    std::vector <vertex_t> idfrom = return_idfrom ();
    std::vector <vertex_t> idto = return_idto ();
    std::vector <weight_t> d = return_d ();
    int ss = idfrom.size ();
    int from_here = idfrom.front ();
    std::vector <neighbor> nblist;
    for (int i=0; i<idfrom.size (); i++)
    {
        int idfromi = idfrom [i];
        if (idfromi == from_here)
        {
            nblist.push_back (neighbor (idto [i], d [i]));
        } else
        {
            from_here = idfromi;
            adjlist.push_back (nblist);
            nblist.clear ();
        }
    }
    unsigned num_vertices = adjlist.size ();

    return num_vertices;
}

/************************************************************************
 ************************************************************************
 **                                                                    **
 **                            DIJKSTRA                                **
 **                                                                    **
 ************************************************************************
 ************************************************************************/

void Graph::Dijkstra (vertex_t source,
        std::vector <weight_t> &min_distance,
        std::vector <vertex_t> &previous)
{
    int n = adjlist.size();
    min_distance.clear();
    min_distance.resize (n, max_weight);
    min_distance [source] = 0;
    previous.clear();
    previous.resize (n, -1);
    std::set <std::pair <weight_t, vertex_t> > vertex_queue;
    vertex_queue.insert (std::make_pair (min_distance [source], source));

    while (!vertex_queue.empty()) 
    {
        weight_t dist = vertex_queue.begin()->first;
        vertex_t u = vertex_queue.begin()->second;
        vertex_queue.erase (vertex_queue.begin());

        // Visit each edge exiting u
        const std::vector <neighbor> &neighbors = adjlist [u];
        for (std::vector <neighbor>::const_iterator neighbor_iter = neighbors.begin();
                neighbor_iter != neighbors.end(); neighbor_iter++)
        {
            vertex_t v = neighbor_iter->target;
            weight_t weight = neighbor_iter->weight;
            weight_t distance_through_u = dist + weight;
            if (distance_through_u < min_distance [v]) {
                vertex_queue.erase (std::make_pair (min_distance [v], v));

                min_distance [v] = distance_through_u;
                previous [v] = u;
                vertex_queue.insert (std::make_pair (min_distance [v], v));

            }

        }
    }
}

/************************************************************************
 ************************************************************************
 **                                                                    **
 **                         GETSHORTESTPATHTO                          **
 **                                                                    **
 ************************************************************************
 ************************************************************************/

std::vector <vertex_t> Graph::GetShortestPathTo (vertex_t vertex, 
        const std::vector <vertex_t> &previous)
{
    std::vector <vertex_t> path;
    for ( ; vertex != -1; vertex = previous [vertex])
        path.push_back (vertex);
    std::reverse (path.begin(), path.end());
    return path;
}

/************************************************************************
 ************************************************************************
 **                                                                    **
 **                            MAKECOSTMAT                             **
 **                                                                    **
 ************************************************************************
 ************************************************************************/

void Graph::make_cost_mat ()
{
    /* the diagonal of cost_mat is 0, otherwise the first row contains only one
     * finite entry for escape from start_node. The last column similarly
     * contains only one finite entry for absorption by end_node. */
    const unsigned num_vertices = return_num_vertices ();
    const unsigned start_node = return_start_node ();
    const unsigned end_node = return_end_node ();

    cost_mat = boost::numeric::ublas::scalar_matrix <weight_t> 
        (num_vertices + 2, num_vertices + 2, max_weight);
    // set diagonal to 0:
    for (unsigned i = 0; i < cost_mat.size1 (); i++)
        cost_mat (i, i) = 0.0;
    // set weights from start_node and to end_node:
    cost_mat (0, start_node + 1) = 0.0;
    cost_mat (end_node + 1, cost_mat.size1 () - 1) = 0.0;

    for (int i=0; i<num_vertices; i++)
    {
        const std::vector <neighbor> &nbs = adjlist [i];
        for (std::vector <neighbor>::const_iterator nb_iter = nbs.begin ();
                nb_iter != nbs.end (); nb_iter++)
        {
            size_t d = std::distance (nbs.begin (), nb_iter);
            // TODO: Implement one-way
            cost_mat (i + 1, d + 1) = cost_mat (d + 1, i + 1) = nb_iter->weight;
        }
    }
}