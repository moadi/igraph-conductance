#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <algorithm>

#include "graph.h"
#include "community.h"

struct less_than_key
{
	inline bool operator() (const pair<int, double>& p1, const pair<int, double>& p2)
	{
		return p1.first < p2.first;
	}
};

int read_communities(std::vector<int>& n2c, std::string input_communities)
{
    ifstream fin(input_communities.c_str());
    
    if (!fin)
    {
        std::cout << "File " << input_communities << " does not exist!\n\n";
        exit(EXIT_FAILURE);
    }
    
    int n; // holds the community assignment
    int i = 0; // index element in vector, has 1:1 correspondence with num elements in data file
    int max = 0; // total number of communities in the graph
    
    // community numbering in igraph starts at 1, so we decrement n by 1
    while(fin >> n)
    {
        n2c[i++] = --n;
        if(n > max)
        {
            max = n;
        }
    }
    fin.close();
    return max + 1;
}

//void update_crossing_edges(std::unordered_map<pair<int, int>, int>& crossing_edges, Graph& g, std::vector<int>& n2c)
//{
//    for (auto it = g.edges.begin(); it != g.edges.end(); it++)
//    {
//        // read in the edge end points
//        int a = it->first.first;
//        int b = it->first.second;
//        
//        
//    }
//}


// read the community assignment of a node, and add it to its community object
void build_communities(std::vector<Community>& c, std::vector<int>& n2c)
{
    for (int i = 0; i < n2c.size(); i++)
    {
        c[n2c[i]].orig_nodes.push_back(i);
    }
}

void build_neighbors(std::vector<Community>& c, std::unordered_map<pair<int, int>, int>& crossing_edges, Graph& g, std::              vector<int>& n2c)
{
    for (auto it = g.edges.begin(); it != g.edges.end(); it++)
    {
        int a = n2c[it->first.first];
        int b = n2c[it->first.second];
        
        // if both nodes are in the same community, do nothing
        if (a == b)
        {
            continue;
        }
        
        pair<int, int> edge;
        if (a < b)
        {
            edge = make_pair(a, b);
        }
        else
        {
            edge = make_pair(b, a);
        }
        
        auto cross_edges_it = crossing_edges.find(edge);
        
        // if key currently does not exist, first time the edge is being encountered
        // insert into the map, and update the neighbors for the communities
        if (cross_edges_it == crossing_edges.end())
        {
            crossing_edges.insert(make_pair(edge, 1));
            c[a].neighbors.push_back(b);
            c[b].neighbors.push_back(a);
        }
        
        else
        {
            cross_edges_it->second++;
        }
        
    }
}

void calc_conductance(std::vector<Community>& c, std::unordered_map<pair<int, int>, int>& crossing_edges, Graph& g, std::string output_file)
{
    std::vector<pair<int, double>> conductance;
    double mean_phi = 0;
    double min_conductance = 99.0;
    double max_conductance = 0;
    
    for (int i = 0; i < c.size(); i++)
    {
        unsigned long long sum_of_degrees = 0;
        int total_crossing_edges = 0;
        
        for (int j = 0; j < c[i].orig_nodes.size(); j++)
        {
            sum_of_degrees += g.vertex[c[i].orig_nodes[j]].degree;
        }
        
        for (int j = 0; j < c[i].neighbors.size(); j++)
        {
            pair<int, int> edge;
            if (i < c[i].neighbors[j])
            {
                edge = make_pair(i, c[i].neighbors[j]);
            }
            else
            {
                edge = make_pair(c[i].neighbors[j], i);
            }
            auto it = crossing_edges.find(edge);
            total_crossing_edges += it->second;
        }
        
        double phi = (double) total_crossing_edges / std::min(g.degree_sum - sum_of_degrees, sum_of_degrees);
        
        if (phi == 0)
        {
            continue;
        }
        if (phi > max_conductance)
        {
            max_conductance = phi;
        }
        if (phi < min_conductance)
        {
            min_conductance = phi;
        }
        
        mean_phi += phi;
        conductance.push_back(make_pair(c[i].orig_nodes.size(), phi));
    }
    std::sort(conductance.begin(), conductance.end(), less_than_key());
    
    ofstream fout(output_file.c_str());
    
    double squared_sum = 0;
    
    mean_phi = (double) mean_phi/conductance.size();
    
    for (int i = 0; i < conductance.size(); i++)
    {
        squared_sum += std::pow((mean_phi - conductance[i].second), 2);
    }
    double std_dev = sqrt((squared_sum) / conductance.size());
    
    double cv = (std_dev / mean_phi) * 100;
    
    fout << "Mean: " << mean_phi << "\n";
    fout << "Maximum: " << max_conductance << "\n";
    fout << "Minimum: " << min_conductance << "\n";
    fout << "Stddev: " << std_dev << "\n";
    fout << "CV: " << cv << "\n\n";
    
    for (auto it = conductance.begin(); it != conductance.end(); it++)
    {
        fout << it->first << "\t" << it->second << "\n";
    }
    fout.close();
}

void write_communities(std::vector<Community>& c, std::string output_file)
{
    ofstream fout(output_file.c_str());
    for (int i = 0; i < c.size(); i++)
    {
        std::sort(c[i].orig_nodes.begin(), c[i].orig_nodes.end());
        for (int j = 0; j < c[i].orig_nodes.size(); j++)
        {
            fout << c[i].orig_nodes[j] + 1 << " ";
        }
        fout << "\n\n\n";
    }
    std::cout << c.size() << "\n\n";
}


int main(int argc, char * argv[])
{
    if (argc != 4)
    {
        std::cout << "Not enough parameters!\n\n";
        std::cout << "Usage: ./conductance <input-graph> <input-communities> <output-file>\n\n";
        exit(EXIT_FAILURE);
    }
    
    char * input_file = argv[1];
    std::string input_communities(argv[2]);
    std::string output_file(argv[3]);
    
    Graph g(input_file);
    
    std::vector<int> n2c(g.num_vertices);
    
    int num_communities = read_communities(n2c, input_communities);
    
    std::vector<Community> c(num_communities);
    
    std::unordered_map<pair<int, int>, int> crossing_edges;
    
    build_communities(c, n2c);
    
    build_neighbors(c, crossing_edges, g, n2c);
    
    calc_conductance(c, crossing_edges, g, output_file);
    
//    for (int i = 0; i < c.size(); i++)
//    {
//        for (int j = 0; j < c[i].neighbors.size(); j++)
//        {
//            std::cout << c[i].neighbors[j] << "\n";
//        }
//        std::cout << "\n\n\n";
//    }
//    write_communities(c, output_file);
    return 0;
}
