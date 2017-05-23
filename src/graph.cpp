#include <Rcpp.h>
#include <algorithm>
#include <vector>
#include <map>

typedef std::string osm_id_t;
typedef int osm_edge_id_t;

struct osm_vertex_t
{
    private:
        std::set <osm_id_t> in, out;
        double lat, lon;

    public:
        bool in_compact_graph = true;
        void add_neighbour_in (osm_id_t osm_id) { in.insert (osm_id); }
        void add_neighbour_out (osm_id_t osm_id) { out.insert (osm_id); }
        int get_degree_in () { return in.size (); }
        int get_degree_out () { return out.size (); }
        void set_lat (double lat) { this -> lat = lat; }
        void set_lon (double lon) { this -> lon = lon; }
        double getLat () { return lat; }
        double getLon () { return lon; }
        std::set <osm_id_t> get_neighbours_in () {return in; }
        std::set <osm_id_t> get_neighbours_out () {return out; }
        std::set <osm_id_t> get_all_neighbours ()
        {
            std::set <osm_id_t> all_neighbours = in;
            all_neighbours.insert (out.begin (), out.end ());
            return all_neighbours;
        }
        void replace_neighbour (osm_id_t n_old, osm_id_t n_new)
        {
            if (in.find (n_old) != in.end ())
            {
                in.erase (n_old);
                in.insert (n_new);
            }
            if (out.find (n_old) != out.end ())
            {
                out.erase (n_old);
                out.insert (n_new);
            }
        }
        bool is_intermediate_single ()
        {
            return (in.size () == 1 && out.size () == 1 &&
                    get_all_neighbours ().size () == 2);
        }
        bool is_intermediate_double ()
        {
            return (in.size () == 2 && out.size () == 2 &&
                    get_all_neighbours ().size () == 2);
        }
};

struct osm_edge_t
{
    private:
        osm_id_t from, to;
        osm_edge_id_t id;
        std::set <int> replacing_edges;
        bool in_original_graph;

    public:
        float dist;
        float weight;
        bool in_compact_graph = true;
        std::string highway;
        osm_id_t get_from_vertex () { return from; }
        osm_id_t getToVertex () { return to; }
        osm_edge_id_t getID () { return id; }
        std::set <int> is_replacement_for () { return replacing_edges; }
        bool in_original () { return in_original_graph; }

        osm_edge_t (osm_id_t from_id, osm_id_t to_id, float dist, float weight,
                   std::string highway, int id, std::set <int> is_rep_for,
                   bool in_original)
        {
            this -> to = to_id;
            this -> from = from_id;
            this -> dist = dist;
            this -> weight = weight;
            this -> highway = highway;
            this -> id = id;
            this -> replacing_edges.insert (is_rep_for.begin (),
                    is_rep_for.end ());
            this -> in_original_graph = in_original;
        }
};

typedef std::map <osm_id_t, osm_vertex_t> vertex_map;
typedef std::vector <osm_edge_t> edge_vector;
typedef std::map <int, std::set <int>> replacement_map;
int edge_ids = 1;

void graph_from_df (Rcpp::DataFrame gr, vertex_map &vm, edge_vector &e)
{
    edge_ids = 1;
    Rcpp::StringVector from = gr ["from_id"];
    Rcpp::StringVector to = gr ["to_id"];
    Rcpp::NumericVector from_lon = gr ["from_lon"];
    Rcpp::NumericVector from_lat = gr ["from_lat"];
    Rcpp::NumericVector to_lon = gr ["to_lon"];
    Rcpp::NumericVector to_lat = gr ["to_lat"];
    Rcpp::NumericVector dist = gr ["d"];
    Rcpp::NumericVector weight = gr ["d_weighted"];
    Rcpp::StringVector hw = gr ["highway"];

    for (int i = 0; i < to.length (); i ++)
    {
        osm_id_t from_id = std::string (from [i]);
        osm_id_t to_id = std::string (to [i]);

        if (vm.find (from_id) == vm.end ())
        {
            osm_vertex_t fromV = osm_vertex_t ();
            fromV.set_lat (from_lat [i]);
            fromV.set_lon (from_lon [i]);
            vm.insert (std::make_pair(from_id, fromV));
        }
        osm_vertex_t from_vtx = vm.at (from_id);
        from_vtx.add_neighbour_out (to_id);
        vm [from_id] = from_vtx;

        if (vm.find (to_id) == vm.end ())
        {
            osm_vertex_t toV = osm_vertex_t ();
            toV.set_lat (to_lat [i]);
            toV.set_lon (to_lon [i]);
            vm.insert (std::make_pair(to_id, toV));
        }
        osm_vertex_t to_vtx = vm.at (to_id);
        to_vtx.add_neighbour_in (from_id);
        vm [to_id] = to_vtx;

        std::set <int> replacementEdges;
        osm_edge_t edge = osm_edge_t (from_id, to_id, dist [i], weight [i],
                std::string (hw [i]), edge_ids ++, replacementEdges, true);
        e.push_back (edge);
    }
}

void get_largest_graph_component (vertex_map &v, std::map <osm_id_t, int> &com,
        int &largest_id)
{
    int component_number = 0;
    // initialize components map
    for (auto it = v.begin (); it != v.end (); ++ it)
        com.insert (std::make_pair (it -> first, -1));

    for (auto it = v.begin (); it != v.end (); ++ it)
    {
        std::set <int> comps;
        osm_id_t vtxId = it -> first;
        osm_vertex_t vtx = it -> second;
        std::set <osm_id_t> neighbors = vtx.get_all_neighbours ();
        comps.insert (com.at (vtxId));
        for (auto n:neighbors)
            comps.insert (com.at (n));
        int largest_comp_num = *comps.rbegin ();
        if (largest_comp_num == -1)
            largest_comp_num = component_number ++;
        com.at (vtxId) = largest_comp_num;
        for (auto n:neighbors)
            com.at (n) = largest_comp_num;
        for (auto c = com.begin (); c != com.end (); ++ c)
        {
            osm_id_t cOsm = c -> first;
            int cNum = c -> second;
            if (comps.find (cNum) != comps.end () && cNum != -1)
                com.at (cOsm) = largest_comp_num;
        }
    }

    std::set <int> unique_components;
    for (auto c:com)
        unique_components.insert (c.second);

    int largest_component_value = -1;
    std::map <int, int> component_size;
    for (auto uc:unique_components)
    {
        int com_size = 0;
        for (auto c:com)
        {
            if (c.second == uc)
                com_size ++;
        }
        if (com_size > largest_component_value)
        {
            largest_component_value = com_size;
            largest_id = uc;
        }
        component_size.insert (std::make_pair (uc, com_size));
    }
}

void remove_small_graph_components (vertex_map &v, edge_vector &e,
        std::map <osm_id_t, int> &components, int &largest_num)
{
    for (auto comp = components.begin (); comp != components.end (); comp ++)
        if (comp -> second != largest_num)
            v.erase (comp -> first);
    auto eIt = e.begin ();
    while (eIt != e.end ())
    {
        osm_id_t fId = eIt -> get_from_vertex ();
        if (v.find (fId) == v.end ())
            eIt = e.erase (eIt);
        else
            eIt ++;
    }
}

void remove_intermediate_vertices (vertex_map &v, edge_vector &e)
{
    auto vert = v.begin ();
    while (vert != v.end ())
    {
        osm_id_t id = vert -> first;
        osm_vertex_t vt = vert -> second;

        std::set <osm_id_t> nIn = vt.get_neighbours_in ();
        std::set <osm_id_t> nOut = vt.get_neighbours_out ();
        std::set <osm_id_t> n_all = vt.get_all_neighbours ();

        bool is_intermediate_single = vt.is_intermediate_single ();
        bool is_intermediate_double = vt.is_intermediate_double ();

        if (is_intermediate_single || is_intermediate_double)
        {
            osm_id_t id_from_new, id_to_new;

            for (auto n_id:n_all)
            {
                osm_id_t replacement_id;
                for (auto repl:n_all)
                    if (repl != n_id)
                        replacement_id = repl;
                osm_vertex_t nVtx = v.at (n_id);
                nVtx.replace_neighbour (id, replacement_id);
                if (is_intermediate_double)
                {
                    id_from_new = n_id;
                    id_to_new = replacement_id;
                }
                v.at (n_id) = nVtx;
            }
            vert ++;

            // update edges
            float dist_new = 0;
            float weight_new = 0;
            std::string hw_new = "";
            int num_found = 0;
            int edges_to_delete = 1;
            if (is_intermediate_double)
                edges_to_delete = 3;
            auto edge = e.begin ();
            while (edge != e.end ())
            {
                if (edge -> in_compact_graph)
                {
                    osm_id_t e_from = edge -> get_from_vertex ();
                    osm_id_t e_to = edge -> getToVertex ();
                    if (e_from == id || e_to == id)
                    {
                        if (is_intermediate_single)
                        {
                            if (e_from == id)
                                id_to_new = e_to;
                            if (e_to == id)
                                id_from_new = e_from;
                        }
                        hw_new = edge -> highway;
                        dist_new += edge -> dist;
                        weight_new += edge -> weight;
                        std::set <int> replacing_edges =
                            edge -> is_replacement_for ();
                        if (num_found >= edges_to_delete)
                        {
                            replacing_edges.insert (edge -> getID ());
                            if (is_intermediate_double)
                            {
                                dist_new = dist_new / 2;
                                weight_new = weight_new / 2;
                                osm_edge_t edge_new = osm_edge_t (id_to_new,
                                        id_from_new, dist_new, weight_new,
                                        hw_new, edge_ids ++, replacing_edges,
                                        false);
                                e.push_back (edge_new);
                            }
                            osm_edge_t edge_new = osm_edge_t (id_from_new,
                                    id_to_new, dist_new, weight_new, hw_new,
                                    edge_ids ++, replacing_edges, false);
                            e.push_back (edge_new);

                            edge -> in_compact_graph = false;
                            edge ++;
                            break;
                        } else
                        {
                            edge -> in_compact_graph = false;
                            edge ++;
                            num_found ++;
                        }
                    } else
                        edge ++;
                } else
                    edge ++;
            }
        } else
            vert ++;
    }
}

//' rcpp_make_compact_graph
//'
//' Removes nodes and edges from a graph that are not needed for routing
//'
//' @param graph graph to be processed
//' @return \code{Rcpp::List} containing one \code{data.frame} with the compact
//' graph, one \code{data.frame} with the original graph and one
//' \code{data.frame} containing information about the relating edge ids of the
//' original and compact graph.
//'
//' @noRd
// [[Rcpp::export]]
Rcpp::List rcpp_make_compact_graph (Rcpp::DataFrame graph)
{
    vertex_map vertices;
    edge_vector edges;
    std::map <osm_id_t, int> components;
    int largest_component;

    graph_from_df (graph, vertices, edges);
    get_largest_graph_component (vertices, components, largest_component);
    remove_small_graph_components (vertices, edges, components,
            largest_component);
    remove_intermediate_vertices (vertices, edges);

    Rcpp::StringVector from_compact, to_compact, highway_compact, from_og,
        to_og, highway_og;
    Rcpp::NumericVector from_lat_compact, from_lon_compact, to_lat_compact,
    to_lon_compact, dist_compact, weight_compact, edgeid_compact, from_lat_og,
    from_lon_og, to_lat_og, to_lon_og, dist_og, weight_og, edgeid_og;

    replacement_map rep_map;

    for (auto e:edges)
    {
        osm_id_t from = e.get_from_vertex ();
        osm_id_t to = e.getToVertex ();
        osm_vertex_t from_vtx = vertices.at (from);
        osm_vertex_t to_vtx = vertices.at (to);
        if (e.in_compact_graph)
        {
            from_compact.push_back (from);
            to_compact.push_back (to);
            highway_compact.push_back (e.highway);
            dist_compact.push_back (e.dist);
            weight_compact.push_back (e.weight);
            from_lat_compact.push_back (from_vtx.getLat ());
            from_lon_compact.push_back (from_vtx.getLon ());
            to_lat_compact.push_back (to_vtx.getLat ());
            to_lon_compact.push_back (to_vtx.getLon ());
            edgeid_compact.push_back (e.getID ());

            std::set <int> rep_edge = e.is_replacement_for ();
            if (rep_edge.size () == 0)
                rep_edge.insert (e.getID ());
            std::set <int> rep_total = rep_map [e.getID ()];
            rep_total.insert (rep_edge.begin (), rep_edge.end ());
            rep_map [e.getID ()] = rep_total;
        }
        if (e.in_original ())
        {
            from_og.push_back (from);
            to_og.push_back (to);
            highway_og.push_back (e.highway);
            dist_og.push_back (e.dist);
            weight_og.push_back (e.weight);
            from_lat_og.push_back (from_vtx.getLat ());
            from_lon_og.push_back (from_vtx.getLon ());
            to_lat_og.push_back (to_vtx.getLat ());
            to_lon_og.push_back (to_vtx.getLon ());
            edgeid_og.push_back (e.getID ());
        }
    }

    Rcpp::NumericVector rp_key, rp_val;
    for (auto rp = rep_map.begin (); rp != rep_map.end (); ++ rp)
    {
        int k = rp -> first;
        std::set <int> v = rp -> second;
        for (auto val:v)
        {
            rp_key.push_back (k);
            rp_val.push_back (val);
        }
    }

    Rcpp::DataFrame compact = Rcpp::DataFrame::create (
            Rcpp::Named ("from_id") = from_compact,
            Rcpp::Named ("to_id") = to_compact,
            Rcpp::Named ("edge_id") = edgeid_compact,
            Rcpp::Named ("d") = dist_compact,
            Rcpp::Named ("d_weighted") = weight_compact,
            Rcpp::Named ("from_lat") = from_lat_compact,
            Rcpp::Named ("from_lon") = from_lon_compact,
            Rcpp::Named ("to_lat") = to_lat_compact,
            Rcpp::Named ("to_lon") = to_lon_compact,
            Rcpp::Named ("highway") = highway_compact);

    Rcpp::DataFrame og = Rcpp::DataFrame::create (
            Rcpp::Named ("from_id") = from_og,
            Rcpp::Named ("to_id") = to_og,
            Rcpp::Named ("edge_id") = edgeid_og,
            Rcpp::Named ("d") = dist_og,
            Rcpp::Named ("d_weighted") = weight_og,
            Rcpp::Named ("from_lat") = from_lat_og,
            Rcpp::Named ("from_lon") = from_lon_og,
            Rcpp::Named ("to_lat") = to_lat_og,
            Rcpp::Named ("to_lon") = to_lon_og,
            Rcpp::Named ("highway") = highway_og);

    Rcpp::DataFrame rel = Rcpp::DataFrame::create (
            Rcpp::Named ("id_compact") = rp_key,
            Rcpp::Named ("id_original") = rp_val);

    return Rcpp::List::create (
            Rcpp::Named ("compact") = compact,
            Rcpp::Named ("original") = og,
            Rcpp::Named ("map") = rel);
}
