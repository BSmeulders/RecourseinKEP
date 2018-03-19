#include "stdafx.h"
#include "Structures.h"
#include "Functions.h"

float Exact_Expected_Transplants(configuration & config);
float Scenarios_Expected_Transpants(configuration & config);

// Function to split pre_test_result into strongly connected components.
// Call to Subset_Weights, to find the expected number of transplants in a SCC.

float Exact_Expected_Transplants(configuration & config)
{
	float Expected_Transplants = 0;
	directedgraph G_test;
	if (config.failure_type == 1)
	{
		G_test = readgraph(config, config.solution_input);
	}
	else if (config.failure_type == 2)
	{
		directedgraph G_orig = readgraph2(config, config.inputfile);
		G_test = readgraph(config, config.solution_input);
		G_test.pairs = G_orig.pairs;
		G_test.ndds = G_orig.ndds;
	}
	cout << "Graph Read" << endl;
	vector<bool> fixed_to_zero (G_test.nr_pairs, 0);
	vector<bool> fixed (G_test.nr_pairs, 0);
	vector<cycle_arcs> SCC = Split_SCC_Tarjan(G_test, fixed_to_zero);
	if (config.failure_type == 1)
	{
		Subset_Set_Weights_Arc_verbose(SCC, G_test, config);
	}
	else if (config.failure_type == 2)
	{
		Subset_Set_Exact_Weights_Vertex(SCC, G_test, config, fixed);
	}
	float total_weight = 0;
	for (int i = 0; i < SCC.size(); i++)
	{
		cout << "SSC " << i << " weight = " << SCC[i].weight << endl;
		total_weight = total_weight + SCC[i].weight;
	}
	ofstream output;
	output.open(config.testvar_output, std::ios_base::app);
	output << "Expected Transplants (Weighted) = " << total_weight << endl;
	return Expected_Transplants;
}

float Scenarios_Expected_Transpants(configuration & config)
{
	float Expected_Tranplants = 0;
	directedgraph G_Orig = readgraph2(config, config.inputfile);
	directedgraph G = readgraph(config, config.solution_input);
	G.pairs = G_Orig.pairs;
	G.ndds = G_Orig.ndds;
	vector<directedgraph> Scenarios;

	if (config.failure_type == 1)
	{
		cout << "Arcs Fail" << endl;
		if (config.scen_gen == 1)
		{
			Scenarios = Generate_Scenarios_Tight(G, config.nr_scenarios); cout << "Tight Scen Generator" << endl;
		}
		else
		{
			Scenarios = Generate_Scenarios(G, config.nr_scenarios); cout << "Basic Scen Generator" << endl;
		}

	}
	else if (config.failure_type == 2)
	{
		cout << "Vertices Fail" << endl;
		Scenarios = Generate_Scenarios_Vertex_Tight(G, config.nr_scenarios);
	}

	// Re-number the arcs, or things break in the HPIEF function. We do save the correct arcnumbers to relate results to the arcs in G.
	for (int i = 0; i < Scenarios.size(); i++)
	{
		for (int j = 0; j < Scenarios[i].arcs.size(); j++)
		{
			Scenarios[i].arcs[j].arcnumber = j;
		}
	}

	int total_transplant = 0;
	for(int i = 0; i < config.nr_scenarios; i++)
	{
		matching_result result = Hybrid_PIEF(Scenarios[i], config.chainlength, config.cyclelength, config);
		total_transplant = result.objective + total_transplant;
	}
	cout << total_transplant << endl;
	cout << float (total_transplant) / float (config.nr_scenarios) << endl;
	Expected_Tranplants = float (total_transplant) / float(config.nr_scenarios);
	ofstream output;
	output.open(config.testvar_output, std::ios_base::app);
	output << "Expected Transplants (Weighted, Approximate) = " << Expected_Tranplants << endl;

	return Expected_Tranplants;
}

float Calc_Expected_Transplants(configuration & config)
{
	float Expected_Transplants;
	if (config.calc_expected_type == 1)
		Expected_Transplants = Exact_Expected_Transplants(config);
	else if (config.calc_expected_type == 2)
		Expected_Transplants = Scenarios_Expected_Transpants(config);

	return Expected_Transplants;
}

vector<cycle_arcs> Split_SCC(const directedgraph & tested_G)
{
	// This functions assumes every arc is part of an SCC.
	vector<cycle_arcs> SCC;
	vector<bool> added(tested_G.size);

	for (int i = 0; i < tested_G.size; i++)
	{
		if (added[i] == 0)
		{
			cout << "Starting SCC with " << i << endl;
			added[i] = 1;
			cycle_arcs new_SCC;
			queue<int> to_test;
			to_test.push(i);
			while (to_test.size() > 0)
			{
				for (int j = 0; j < tested_G.arcs.size(); j++)
				{
					if (tested_G.arcs[j].endvertex >= i)
					{
						if (tested_G.arcs[j].startvertex == to_test.front())
						{
							new_SCC.arcs.push_back(j);
						}
						if (tested_G.arcs[j].startvertex == to_test.front() && added[tested_G.arcs[j].endvertex] == 0)
						{
							added[tested_G.arcs[j].endvertex] = 1;
							to_test.push(tested_G.arcs[j].endvertex);
						}
					}
				}
				new_SCC.vertices.push_back(to_test.front());
				to_test.pop();
			}
			if (new_SCC.vertices.size() > 1)
			{
				SCC.push_back(new_SCC);
				for (int j = 0; j < new_SCC.vertices.size(); j++)
				{
					cout << new_SCC.vertices[j] << "\t";
				}
				cout << endl;
				for (int j = 0; j < new_SCC.arcs.size(); j++)
				{
					cout << "(" << tested_G.arcs[new_SCC.arcs[j]].startvertex << "," << tested_G.arcs[new_SCC.arcs[j]].endvertex << ")" << endl;
				}
				cout << endl;
			}

		}
	}

	return SCC;
}


vector<list<int>> G_Adjacency(const directedgraph & tested_G, const vector<bool> & fixed_to_zero) {
	// We keep track of the arcs linked to v and not exactly vertices as we need the
	// index of the arcs to build the SCCs later on. Furthemore the endvertex can be easily
	// gathered through test_G
	vector<list<int>> adj (tested_G.nr_pairs);
	int sv, ev;
	for (unsigned int i = 0; i < tested_G.arcs.size(); ++i) {
		sv = tested_G.arcs[i].startvertex;
		ev = tested_G.arcs[i].endvertex;
		if (fixed_to_zero[sv] == 0 && fixed_to_zero[ev] == 0) adj[sv].push_back(i);
	}
	return adj;
}

vector<cycle_arcs> Split_SCC_Tarjan(const directedgraph & tested_G, const vector<bool> & fixed_to_zero) {
	vector<cycle_arcs> SCC;
	int index = 0;
	vector<int> vertices_index (tested_G.nr_pairs, -1); // -1 means the index of the vertex is undefined.
	vector<int> low_link (tested_G.nr_pairs, -1);
	vector<bool> on_stack(tested_G.nr_pairs, 0);
	vector<list<int>> adj = G_Adjacency(tested_G, fixed_to_zero);
	deque<int> Q;
	vector<vector<int>> SCC_arcs(tested_G.nr_pairs);
	for (unsigned int v = 0; v < tested_G.nr_pairs; ++v) {
		if (vertices_index[v] == -1) Find_SCC_Root(tested_G, adj, v, index, vertices_index, Q, low_link, on_stack, SCC, SCC_arcs);
	}
	return SCC;
}

vector<tuple<int,int,float>> Find_Articulation_Points(const directedgraph & G) {
	vector<tuple<int, int, float>> articulation_points;
	vector<bool> fixed_to_zero;
	vector<cycle_arcs> SCC;
	float score;
	int max_SCC;
	for (unsigned int i = 0; i < G.nr_pairs; ++i) {
		score = 1;
		fixed_to_zero = vector<bool>(G.nr_pairs, 0);
		fixed_to_zero[i] = 1;
		SCC = Split_SCC_Tarjan(G, fixed_to_zero);
		if (SCC.size() > 1) {
			max_SCC = SCC[0].vertices.size();
			for (unsigned int j = 0; j < SCC.size(); j++) {
				if (SCC[j].vertices.size() > max_SCC) max_SCC = SCC[j].vertices.size();
			}
			articulation_points.push_back(tuple<int, int, float>(i, SCC.size(), max_SCC));
		}
	}
	return articulation_points;
}


void Find_SCC_Root(const directedgraph & tested_G, const vector<list<int>> & adj, int v, int & index, vector<int> & vertices_index,
				   deque<int> & Q, vector<int> & low_link, vector<bool> & on_stack, vector<cycle_arcs> & SCC, vector<vector<int>> & SCC_arcs) {
	vertices_index[v] = index;
	low_link[v] = index;
	index++;
	Q.push_front(v);
	on_stack[v] = true;
	for (list<int>::const_iterator it = adj[v].begin(); it != adj[v].end(); ++it) {
		int w = tested_G.arcs[*it].endvertex;
		if (vertices_index[w] == -1) {
			// (*it) has not been visited yet, explore it to find its low link
			Find_SCC_Root(tested_G, adj, w, index, vertices_index, Q, low_link, on_stack, SCC, SCC_arcs);
			low_link[v] = min(low_link[v], low_link[w]);
			if (on_stack[w]) {
				SCC_arcs[v].push_back(*it);
			}
		}
		else if (on_stack[w]) {
			// (*it) has been visited and is still on the stack, hence belongs to the same SCC
			// either rooted in *it (min = index[*it]) or by a vertex above in the DFS tree (min = low_link[v])
			low_link[v] = min(low_link[v], vertices_index[w]);
			SCC_arcs[v].push_back(*it);
		}
	}

	// Then we pop the vertices to build the SCC
	int w = -1;
	if (low_link[v] == vertices_index[v]) {
		cycle_arcs new_SCC;
		do {
			if (Q.empty()) break;
			w = Q.front();
			Q.pop_front();
			on_stack[w] = 0;
			new_SCC.vertices.push_back(w);
		} while(w != v);

		if (new_SCC.vertices.size() > 1) {
			for (int j = 0; j < new_SCC.vertices.size(); j++) {
				// We concatenate the list of arcs that belongs to the current SCC together
				new_SCC.arcs.insert(new_SCC.arcs.end(), SCC_arcs[new_SCC.vertices[j]].begin(), SCC_arcs[new_SCC.vertices[j]].end());
			}
			SCC.push_back(new_SCC);
		}
	}
	return;
}

void Subset_Set_Exact_Weights_Vertex(vector<cycle_arcs>& subsets, const directedgraph & G, const configuration & config, vector<bool> & fixed)
{
	int count = 0;
#pragma omp parallel for
	for (int i = 0; i < subsets.size(); i++)
	{
		// Build subset graph
		vector<bool> new_fixed (subsets[i].vertices.size(), 0);
		directedgraph G_sub = Subset_Graph_Exact(subsets[i], G, new_fixed, fixed);
		// Give Graph to function computing weight for subset
		subsets[i].weight = Subset_Set_Exact_Weights_Vertex_Root(G_sub, config, new_fixed);
		count++;
		if (count % 100 == 0)
			cout << count << endl;
	}
}

float Subset_Set_Exact_Weights_Vertex_Root(const directedgraph & G, const configuration & config, vector<bool> & fixed)
{
	float weight;

	// We first find all the cycles
	vector<cycle_arcs> cycles = Find_Cycles(G, config);
	if (cycles.size() == 0) return 0;
	if (cycles.size() == 1) {
		float tmp_weight = 1;
		for (unsigned int i = 0; i < cycles[0].vertices.size(); ++i) {
			if (fixed[cycles[0].vertices[i]] == 0) tmp_weight *= (1-G.pairs[cycles[0].vertices[i]].failprob);
		}
		tmp_weight *= cycles[0].vertices.size();
		return tmp_weight;
	}

	// We compute the branching order: first the vertices that are involved in the highest number of cyles
	vector<int> nr_involved_cycles (G.nr_pairs, 0); // Vertex i is involved in nr_involved_cycles[i] cycles
	for (unsigned int i = 0; i < cycles.size(); ++i) {
		for (unsigned j = 0; j < cycles[i].vertices.size(); ++j) nr_involved_cycles[cycles[i].vertices[j]]++;
	}

	int max_involved_cycles = *(max_element(nr_involved_cycles.begin(), nr_involved_cycles.end())); // Upper bound
	vector<list<tuple<int,int>>> nr_appearance_lists (max_involved_cycles + 1); // nr_appearance_lists[i] is the list of vertex involved in exactly i cycles.

	for (unsigned int i = 0; i < nr_involved_cycles.size(); ++i) {
		nr_appearance_lists[nr_involved_cycles[i]].push_back(tuple<int,int>(i, nr_involved_cycles[i]));
	}

	// We concatenate the lists of nr_appearance_lists to obtain the final order
	list<tuple<int,int>> branching_order;
	for (unsigned int i = 0; i < nr_appearance_lists.size(); ++i) {
		if (!nr_appearance_lists[i].empty()) branching_order.splice(branching_order.begin(), nr_appearance_lists[i]);
	}

	// We set up 2 LPs, one for the upper bound (assume all arcs not yet set will succeed) and one for the LB (assume all arcs not yet set will fail).
	IloEnv env;
	IloModel Model(env);
	IloNumVarArray cyclevar(env, cycles.size(), cycles.size(), ILOINT);
	for (int i = 0; i < cycles.size(); i++)
		cyclevar[i] = IloNumVar(env, 0, 1, ILOINT);

	IloRangeArray vertex_cons = Build_Vertex_Constraint_SSWR(env, G, cyclevar, cycles);
	Model.add(vertex_cons);

	// Objective function
	IloObjective obj = IloMaximize(env);
	for (int i = 0; i < cycles.size(); i++)
	{
		obj.setLinearCoef(cyclevar[i], cycles[i].weight);
	}

	Model.add(obj);
	cout << "Model Built" << endl;
	IloCplex CPLEX(Model); CPLEX.setOut(env.getNullStream());
	CPLEX.setWarning(env.getNullStream());
	CPLEX.solve();

	float CPLEX_Solution = ceil(CPLEX.getObjValue());
	vector<int> vertex_use(G.size);
	for (int i = 0; i < G.size; i++)
	{
		vertex_use[i] = 1 - round(CPLEX.getSlack(vertex_cons[i]));
	}

	// We pick one vertex used in the current solution and fix it (either fail or success).
	// If success, the number of transplants will not change. We pick a new vertex used in the solution and branch again.
	// If failure, we resolve, and pick another vertex (in the solution) and branch on it.
	// If there are no vertices used in the solution that are not yet fixed, the branching ends and the current solution is returned.

	vector<tuple<int,int>> vertex_fix (2, tuple<int,int>(-1,-1));
	vector<tuple<int, int, float>> a_p = Find_Articulation_Points(G);

	// We try to find the best possible articulation point and branch on it first.
	if (a_p.size() > 0) {
		sort(a_p.begin(), a_p.end(), Ap_compare);
		for (int i = a_p.size()-1; i >= 0; --i) {
			if (vertex_use[get<0>(a_p[i])] == 1) {
				branching_order.push_front(tuple<int,int>(get<0>(a_p[i]), 0));
				break;
			}
		}
	}

	bool new_fix = 0;
	// We have to know which vertex have failed in order to be able to compute the potential SCC
	// after each new failure.
	vector<bool> fixed_to_zero(G.nr_pairs, 0);

	for(list<tuple<int,int>>::iterator it = branching_order.begin(); it != branching_order.end(); ++it) {
		int id = get<0>(*it);
		if(vertex_use[id] == 1 && fixed[id] == 0) {
			new_fix = 1;
			fixed[id] = 1;
			vertex_fix[0] = *it;
			break;
		}
	}
	if (new_fix == 0) return CPLEX_Solution;

	// We look for a first candidate. Use the line below to find the next vertex to branch on
	// with the branching order. We do not have to use it currently as the branching order never changes
	// in the current version.
	// Find_next_branch(vertex_fix, 0, vertex_use, fixed, branching_order, new_fix);

	cout << "Model Solved, Branching starting" << endl;
	float solution = 0;
	int depth = 0;
	deque<int> suc_fail;

	if (CPLEX_Solution > 0) {
		depth++;
		// First assume the vertex is a succes
		// We go down one level and branch again on another arc used in the solution.
		suc_fail.push_back(1);
		weight = (1 - G.pairs[get<0>(vertex_fix[0])].failprob)*Subset_Set_Exact_Weights_Vertex_Recursion(config, env, G, CPLEX, vertex_use, vertex_cons, fixed, CPLEX_Solution, 1, depth, suc_fail, branching_order, fixed_to_zero);
		suc_fail.pop_back();
		suc_fail.push_back(0);

		// Next assume the vertex fails.
		vertex_cons[get<0>(vertex_fix[0])].setUB(0);
		fixed_to_zero[get<0>(vertex_fix[0])] = 1;
		weight = weight + G.pairs[get<0>(vertex_fix[0])].failprob*Subset_Set_Exact_Weights_Vertex_Recursion(config, env, G, CPLEX, vertex_use, vertex_cons, fixed, CPLEX_Solution, 0, depth, suc_fail, branching_order, fixed_to_zero);
		suc_fail.pop_back();
		vertex_cons[get<0>(vertex_fix[0])].setUB(1);
	}
	env.end();
	return weight;
}


float Subset_Set_Exact_Weights_Vertex_Recursion(const configuration & config, IloEnv & env,
	const directedgraph & G, IloCplex & CPLEX, vector<int> vertex_use, IloRangeArray & vertex_cons, vector<bool> fixed, float Solution,
	bool succes_fix, int depth, deque<int> & suc_fail, list<tuple<int, int>> branching_order, vector<bool> & fixed_to_zero)
{

	float weight = 0;
	if (succes_fix == 0)
	{
		// First see if graph has been broken into several SCCs.
		// If it is the case, we stop the recursion here and start
		// again from root with the found SCCs.
		vector<cycle_arcs> SCC_T = Split_SCC_Tarjan(G, fixed_to_zero);
		if (SCC_T.size() > 1) {
			Subset_Set_Exact_Weights_Vertex(SCC_T, G, config, fixed);
			for (int i = 0; i < SCC_T.size(); i++) weight = weight + SCC_T[i].weight;
			return weight;
		}

		CPLEX.solve();
		Solution = ceil(CPLEX.getObjValue());

		for (int i = 0; i < G.size; i++)
		{
			vertex_use[i] = 1 - round(CPLEX.getSlack(vertex_cons[i]));
		}
	}

	vector<tuple<int,int>> vertex_fix (2, tuple<int,int>(-1,-1));
	bool new_fix = 0;

	for(list<tuple<int,int>>::iterator it = branching_order.begin(); it != branching_order.end(); ++it) {
		int id = get<0>(*it);
		if(vertex_use[id] == 1 && fixed[id] == 0) {
			new_fix = 1;
			fixed[id] = 1;
			vertex_fix[0] = *it;
			break;
		}
	}

	// We look for the first candidate
	//Find_next_branch(vertex_fix, 0, vertex_use, fixed, branching_order, new_fix);

	if (new_fix == 1)
	{
		depth++;
		// First assume the vertex is a success
		// We go down one level and branch again on another arc used in the solution.
		suc_fail.push_back(1);
		weight = (1 - G.pairs[get<0>(vertex_fix[0])].failprob)*Subset_Set_Exact_Weights_Vertex_Recursion(config, env, G, CPLEX, vertex_use, vertex_cons, fixed, Solution, 1, depth, suc_fail, branching_order, fixed_to_zero);
		suc_fail.pop_back();

		// Next assume the vertex fails.
		suc_fail.push_back(0);
		vertex_cons[get<0>(vertex_fix[0])].setUB(0);
		weight = weight + G.pairs[get<0>(vertex_fix[0])].failprob*Subset_Set_Exact_Weights_Vertex_Recursion(config, env, G, CPLEX, vertex_use, vertex_cons, fixed, Solution, 0, depth, suc_fail, branching_order, fixed_to_zero);
		suc_fail.pop_back();
		vertex_cons[get<0>(vertex_fix[0])].setUB(1);
	}
	else // If there are no more arcs to fix, the weight is equal to the last CPLEX Solution (all arcs used in this solution have been fixed to success).
	{
		weight = Solution;
	}

	return weight;
}

// Just a few modification made on the function "Subset_Graph" of the subset recourse module. Indeed, when breaking the graph with SCC and starting
// with new tree, the vertices get reindexed. Hence we also have to reindex the vertex that have already been
// fixed so that we do not branch twice on the same vertex.
// Not that we do not have to keep track of the fixed_to_zero vertices has these necessarily disapear
// when breaking the graph.
directedgraph Subset_Graph_Exact(const cycle_arcs & subset, const directedgraph & G, vector<bool> & new_fixed, vector<bool> & fixed)
{
	directedgraph G_sub;
	G_sub.size = subset.vertices.size();
	G_sub.nr_pairs = G_sub.size;
	vector<int> vertex_switch(G.nr_pairs); // For each vertex in the original graph, we save in which position it occurs, in the list of vertices in the subset.
	for (int i = 0; i < subset.vertices.size(); i++)
	{
		vertex_switch[subset.vertices[i]] = i;
		new_fixed[i] = fixed[subset.vertices[i]];
		G_sub.pairs.push_back(G.pairs[subset.vertices[i]]);
	}
	for (int i = 0; i < subset.arcs.size(); i++)
	{
		G_sub.arcs.push_back(G.arcs[subset.arcs[i]]);
		G_sub.arcs[i].startvertex = vertex_switch[G_sub.arcs[i].startvertex];
		G_sub.arcs[i].endvertex = vertex_switch[G_sub.arcs[i].endvertex];
	}

	sort(G_sub.arcs.begin(), G_sub.arcs.end(), arcsort);
	Find_First_arc(G_sub);
	return G_sub;
}

// Function to sort the articulations points. They are represented as tuple of 2 int and 1 float.
// The first integer is the vertex ID, the second integer is the number of SCCs linked to this articulation.
// point. The third number is currently the size of the biggest SCC linked to this point (if removed from graph)
// It has been set as a float so that it can easily be replaced by some other measure that could be more relevant
// to choose the point to branch on. We use currently a lexicographic order that first aim to maximize the number
// of SCCs and then to minimize the size of the biggest one.
// NB: the order is reversed so that one can simply push these point on the front of the current order
bool Ap_compare(tuple<int, int, float> const & t1, tuple<int, int, float> const & t2) {
	// If same size, we give priority to the smallest one. Not necessary equal as one may
	// have deleted lone vertices when branching
	if (get<1>(t1) == get<1>(t2)) return get<2>(t1) >= get<2>(t2);
	return get<1>(t1) > get<2>(t1);
}
