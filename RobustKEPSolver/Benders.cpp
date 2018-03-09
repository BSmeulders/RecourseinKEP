#include "stdafx.h"
#include "Structures.h"
#include "Functions.h"

pre_test_result Bender(directedgraph G, configuration & config);
pre_test_result Bender_HPIEF(directedgraph G, configuration &config);
IloNumVarArray Generate_Cycle_Var_Cycle_Form_Linear(IloEnv & env, const directedgraph & Scenario_Graph, const configuration & config, int i);
IloRangeArray Generate_Vertex_Use_Constraint(IloEnv & env, const directedgraph & G, const vector<cycle_arcs> & cycles, IloNumVarArray & cyclevar, int scen);
IloRangeArray Generate_Test_Cycle_Constraint(IloEnv & env, IloNumVarArray & cyclevar, IloNumVarArray & testvar, const vector<cycle_arcs> & cycles, int scen);

cycle_variables Generate_Cycle_Var_LP(IloEnv &env, const directedgraph & G, int cyclelength, int nr_scen);
chain_variables Generate_Chain_Var_LP(IloEnv & env, directedgraph G, int chainlength, int nr_scen);

vector<cycle_arcs> Find_Cycles_Bender(directedgraph G, const configuration & config);

void benders_main(configuration & config, directedgraph G)
{
	pre_test_result results = Bender_HPIEF(G, config);
	Output_Pre_Test(results, config);
}

pre_test_result Bender(directedgraph G, configuration & config)
{
	directedgraph Tested_Graph = G;
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
	cout << "Scenarios Generated" << endl;
	cout << Scenarios.size();


	time_t start_time;
	time(&start_time);
	IloEnv env;
	IloModel model(env);
	cout << "Generating Variables" << endl;
	// Create the variables.
	// Variables per cycle for each scenario.
	vector<vector<cycle_arcs>> cycles(config.nr_scenarios);
	vector<IloNumVarArray> cyclevar(config.nr_scenarios);
	for (int i = 0; i < config.nr_scenarios; i++)
	{
		// Note! Problem with Find_Cycles.
		cycles[i] = Find_Cycles_Bender(Scenarios[i], config);
		cyclevar[i] = IloNumVarArray(env, cycles[i].size(), 0, 1, ILOFLOAT);
	}
	cout << "Scenario Var created" << endl;
	// Create Testing Variables and Constraints.
	IloNumVarArray Testvar = Generate_Testvar(env, G);
	cout << "Testvar created" << endl;
	
	// Create the Objective Function
	IloObjective obj = IloMaximize(env);
	for (int scen = 0; scen < config.nr_scenarios; scen++)
	{
		cout << scen << endl;
		cout << "Cycles size = " << cycles[scen].size() << endl;
		cout << "Cyclevar size = " << cyclevar[scen].getSize() << endl;
		for (int i = 0; i < cycles[scen].size(); i++)
		{
			cout << scen << "\t" << i << endl;
			obj.setLinearCoef(cyclevar[scen][i], cycles[scen][i].weight);
		}
	}
	model.add(obj);
	cout << "Objective function done" << endl;
	// Create Constraints per scenario.
	// Vertex Constraints
	vector<IloRangeArray> vertex_cons(config.nr_scenarios);
	for (int i = 0; i < config.nr_scenarios; i++)
	{
		vertex_cons[i] = Generate_Vertex_Use_Constraint(env, G, cycles[i], cyclevar[i], i);
		model.add(vertex_cons[i]);
	}
	// Cycle only used if all Testvar used.
	vector<IloRangeArray> testconstraint(config.nr_scenarios);
	for (int i = 0; i < config.nr_scenarios; i++)
	{
		testconstraint[i] = Generate_Test_Cycle_Constraint(env, cyclevar[i], Testvar, cycles[i], i);
		model.add(testconstraint[i]);
	}
	IloRange Max_Test_Constraint = Build_Max_Test_Constraint(env, model, Testvar, config.max_test);
	model.add(Max_Test_Constraint);

	IloCplex CPLEX(model);
	CPLEX.setParam(IloCplex::TiLim, config.time_limit);
	CPLEX.setParam(IloCplex::TreLim, config.memory_limit);
	CPLEX.setParam(IloCplex::Param::Benders::Strategy, IloCplex::BendersFull);
	CPLEX.solve();

	pre_test_result results;
	results.objective_value = CPLEX.getObjValue() / config.nr_scenarios;
	cout << results.objective_value << endl;

	time_t current_time;
	time(&current_time);
	results.computation_time = difftime(current_time, start_time);

	for (int i = 0; i < G.arcs.size(); i++)
	{
		if (CPLEX.getValue(Testvar[i]) > 0.99)
		{
			results.tested_arcs.push_back(G.arcs[i]);
		}
	}
	return results;
}

IloNumVarArray Generate_Cycle_Var_Cycle_Form_Linear(IloEnv & env, const directedgraph & Graph, const configuration & config, int scen_number)
{
	



	return IloNumVarArray();
}

IloRangeArray Generate_Vertex_Use_Constraint(IloEnv & env, const directedgraph & G, const vector<cycle_arcs>& cycles, IloNumVarArray& cyclevar, int scen)
{
	IloRangeArray constraint(env, G.size);
	for (int i = 0; i < G.size; i++)
	{
		IloExpr expr(env);
		constraint[i] = IloRange(expr <= 1);
		ostringstream convert;
		convert << "S." << scen << ",V." << i;
		string varname = convert.str();
		const char* vname = varname.c_str();
		constraint[i].setName(vname);
	}
	for (int j = 0; j < cycles.size(); j++)
	{
		for (int k = 0; k < cycles[j].vertices.size(); k++)
		{
			constraint[cycles[j].vertices[k]].setLinearCoef(cyclevar[j], 1);
		}
	}
	return constraint;
}

IloRangeArray Generate_Test_Cycle_Constraint(IloEnv & env, IloNumVarArray & cyclevar, IloNumVarArray & testvar, const vector<cycle_arcs> & cycles, int scen)
{
	IloRangeArray constraint(env, testvar.getSize());

	for (int i = 0; i < testvar.getSize(); i++)
	{
		IloExpr expr(env);
		constraint[i] = IloRange(expr >= 0);
		ostringstream convert;
		convert << "S." << scen << ",T." << i;
		string varname = convert.str();
		const char* vname = varname.c_str();
		constraint[i].setName(vname);
		constraint[i].setLinearCoef(testvar[i], 1);
	}
	for (int j = 0; j < cycles.size(); j++)
	{
		for (int k = 0; k < cycles[j].arcs.size(); k++)
		{
			constraint[cycles[j].arcs[k]].setLinearCoef(cyclevar[j], -1);
		}
	}
	
	return constraint;
}

pre_test_result Bender_HPIEF(directedgraph G, configuration &config)
{
	directedgraph Tested_Graph = G;
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
	cout << "Scenarios Generated" << endl;
	cout << Scenarios.size();


	time_t start_time;
	time(&start_time);
	IloEnv env;
	IloModel model(env);
	cout << "Generating Variables" << endl;
	vector<vector<vector<IloNumVarArray>>> Cyclevar(config.nr_scenarios); // First position is scenario, second index is the Graph Copy, third the position in the Graph, fourth the individual arcs.
	vector<vector<vector<vector<int>>>> Cyclevar_arc_link(config.nr_scenarios); // A vector to link the variables to the original arc. Cyclevar_arc_link[i][j][k][l] = m, means that this variable corresponds to the m-th arc in the original arc list.
	for (int i = 0; i < config.nr_scenarios; i++)
	{
		cycle_variables cvars = Generate_Cycle_Var_LP(env, Scenarios[i], config.cyclelength, i);
		Cyclevar[i] = cvars.Cyclevariable;
		Cyclevar_arc_link[i] = cvars.Link_Cyclevar_Arc;
	}
	cout << "Cyclevar Generated" << endl;
	vector<vector<IloNumVarArray>> Chainvar(config.nr_scenarios); // First index is the scenario, second index is the position in the graph, third the individual arc.
	vector<vector<vector<int>>> Chainvar_arc_link(config.nr_scenarios); // A vector to link the variables to the original arc.
	for (int i = 0; i < config.nr_scenarios; i++)
	{
		chain_variables cvars = Generate_Chain_Var_LP(env, Scenarios[i], config.chainlength, i);
		Chainvar[i] = cvars.Chainvar;
		Chainvar_arc_link[i] = cvars.Link_Chainvar_Arc;
	}
	cout << "Chainvar Generated" << endl;
	cout << "Variables Generated" << endl;
	// Create the Objective Function
	IloObjective obj = IloMaximize(env);
	for (int scen = 0; scen < config.nr_scenarios; scen++)
	{
		for (int i = 0; i < G.nr_pairs - 1; i++)
		{
			for (int j = 0; j < config.cyclelength; j++)
			{
				for (int k = 0; k < Cyclevar[scen][i][j].getSize(); k++)
				{
					obj.setLinearCoef(Cyclevar[scen][i][j][k], G.arcs[Cyclevar_arc_link[scen][i][j][k]].weight);
				}
			}
		}
		for (int i = 0; i < config.chainlength; i++)
		{
			for (int j = 0; j < Chainvar[scen][i].getSize(); j++)
				obj.setLinearCoef(Chainvar[scen][i][j], G.arcs[Chainvar_arc_link[scen][i][j]].weight);
		}
	}
	model.add(obj);
	cout << "Objective Created" << endl;
	// Create Constraints per scenario.
	vector<IloRangeArray> vertex_inflow_cons(config.nr_scenarios);
	vector<vector<vector<IloRangeArray>>> vertex_flow_cons(config.nr_scenarios);
	vector<vector<IloRangeArray>> vertex_chain_flow_cons(config.nr_scenarios);
	vector<IloRangeArray> NDD_Constraint(config.nr_scenarios);
	for (int scen = 0; scen < config.nr_scenarios; scen++)
	{
		if (scen % 100 == 0)
			cout << "Generating constraints for scenario " << scen << endl;
		// Max one incoming arc per vertex.
		vertex_inflow_cons[scen] = Build_Vertex_Constraint(env, model, G, Cyclevar[scen], Cyclevar_arc_link[scen], Chainvar[scen], Chainvar_arc_link[scen]);

		// If there is an arc arriving in position i, there should be an outgoing arc in position i+1 in the same copy (excluding the origin vertex in that copy).
		vertex_flow_cons[scen] = Build_Vertex_Flow_Constraint(env, model, G, Cyclevar[scen], Cyclevar_arc_link[scen], config.cyclelength);

		// If there is an outgoing arc_chain in position i, there should be an incoming arc_chain in position [i-1].
		vertex_chain_flow_cons[scen] = Build_Vertex_Flow_Chain_Constraint(env, model, G, Chainvar[scen], Chainvar_arc_link[scen], config.chainlength);

		// At most one outgoing arc for each NDD
		NDD_Constraint[scen] = Build_NDD_Constraint(env, model, G, Chainvar[scen], Chainvar_arc_link[scen]);
	}
	// Create Testing Variables and Constraints.
	IloNumVarArray Testvar = Generate_Testvar(env, G);
	vector<IloRangeArray> test_constraint = Build_Test_Constraint(env, model, G, Testvar, Cyclevar, Cyclevar_arc_link, Chainvar, Chainvar_arc_link, config.nr_scenarios);
	IloRange Max_Test_Constraint = Build_Max_Test_Constraint(env, model, Testvar, config.max_test);

	IloCplex CPLEX(model);
	CPLEX.setParam(IloCplex::TiLim, config.time_limit);
	CPLEX.setParam(IloCplex::TreLim, config.memory_limit);
	CPLEX.setParam(IloCplex::Param::Benders::Strategy, IloCplex::BendersFull);
	CPLEX.solve();

	pre_test_result results;
	results.objective_value = CPLEX.getObjValue() / config.nr_scenarios;
	cout << results.objective_value << endl;

	time_t current_time;
	time(&current_time);
	results.computation_time = difftime(current_time, start_time);

	for (int i = 0; i < G.arcs.size(); i++)
	{
		if (CPLEX.getValue(Testvar[i]) > 0.99)
		{
			results.tested_arcs.push_back(G.arcs[i]);
		}
	}


	return results;
}

cycle_variables Generate_Cycle_Var_LP(IloEnv &env, const directedgraph & G, int cyclelength, int nr_scen)
{
	// Note that we consistently work with nr_pairs - 1. Since the copy corresponding to the last pair is empty (no arcs), it is useless to include it.
	cycle_variables c;
	c.Cyclevariable.resize(G.nr_pairs - 1);
	c.Link_Cyclevar_Arc.resize(G.nr_pairs - 1);
	// Pre-Processing
	vector<vector<directedarc>> Acopies = DP_Copy(G);
	vector<vector<vector<int>>> copy_pos_arc_possible = cycle_preproces(G, Acopies, cyclelength);

	// Create all variables
	{
		//ostringstream convert;
		for (int i = 0; i < G.nr_pairs - 1; i++)
		{
			c.Link_Cyclevar_Arc[i].resize(cyclelength);
			for (int j = 0; j < cyclelength; j++)
			{
				c.Cyclevariable[i].push_back(IloNumVarArray(env));
				for (int k = 0; k < Acopies[i].size(); k++)
				{
					if (copy_pos_arc_possible[i][j][k] == 1)
					{
						//convert << "x(" << nr_scen << "," << Acopies[i][k].startvertex << "," << Acopies[i][k].endvertex << "," << i << "," << j << ")";
						//string varname = convert.str();
						//const char* vname = varname.c_str();
						c.Cyclevariable[i][j].add(IloNumVar(env, 0, 1, ILOFLOAT/*, vname*/));
						c.Link_Cyclevar_Arc[i][j].push_back(Acopies[i][k].arcnumber);
						//convert.str("");
						//convert.clear();
					}
				}
			}
		}
	}
	return c;
}

chain_variables Generate_Chain_Var_LP(IloEnv & env, directedgraph G, int chainlength, int nr_scen)
{
	chain_variables c;
	c.Link_Chainvar_Arc.resize(G.nr_pairs - 1);

	// Pre-Processing. For each arc of G, the following function checks whether they can be in a particular position in any feasible solution.
	vector<vector<int>> arc_position_possible = chain_preproces(G, chainlength);
	// The first index is Position, the second is the individual arc.

	// Create all variables
	for (int i = 0; i < chainlength; i++)
	{
		c.Chainvar.push_back(IloNumVarArray(env));
		for (int j = 0; j < G.arcs.size(); j++)
		{
			if (arc_position_possible[i][j] == 1)
			{
				if (i == 0 && G.arcs[j].startvertex >= G.nr_pairs) // If this is true, the starvertex is an NDD and can be placed in the first position of a chain.
				{
					ostringstream convert;
					convert << "y(" << nr_scen << "," << G.arcs[j].startvertex << "," << G.arcs[j].endvertex << "," << i << ")";
					string varname = convert.str();
					const char* vname = varname.c_str();
					c.Chainvar[i].add(IloNumVar(env, 0, 1, ILOFLOAT, vname));
					c.Link_Chainvar_Arc[i].push_back(G.arcs[j].arcnumber);
				}
				else if (i > 0 && G.arcs[j].startvertex < G.nr_pairs) // Any arc between pairs can be used in the following positions
				{
					ostringstream convert;
					convert << "y(" << G.arcs[j].startvertex << "," << G.arcs[j].endvertex << "," << i << ")";
					string varname = convert.str();
					const char* vname = varname.c_str();
					c.Chainvar[i].add(IloNumVar(env, 0, 1, ILOINT, vname));
					c.Link_Chainvar_Arc[i].push_back(G.arcs[j].arcnumber);
				}
			}
		}
	}

	return c;
}

vector<cycle_arcs> Find_Cycles_Bender(directedgraph G, const configuration & config)
{
	// This function identifies all cycles in the graph.
	vector<cycle_arcs> cycles;

	vector<vector<directedarc>> copies = DP_Copy(G);
	vector<vector<int>> distances = distance_calc(G, copies, config.cyclelength);
	vector<int> first_arc = Find_First_arc(G);

	for (int i = 0; i < G.nr_pairs; i++) // We loop through all vertices as possible starting points of the cycles.
	{
		queue<vector<int>> paths; // A queue (to allow deleting elements at the front) including all current paths.
								  // We intialize the queue by adding all arcs from the initial vertex (if a short enough return path exists).
		for (int j = first_arc[i]; j < first_arc[i + 1]; j++)
		{
			if (distances[i][G.arcs[j].endvertex] < config.cyclelength) // Note that due to the distance calc, no arcs to lower numbered vertices will be used in this initialization (since return distance = cyclelength + 1);
			{
				vector<int> init_vector;
				init_vector.push_back(j);
				paths.push(init_vector);
			}
		}
		while (!paths.empty())
		{
			vector<int> path_vector = paths.front(); // Get the first path out and delete it from the queue.
			paths.pop();

			int endvertex = G.arcs[path_vector[path_vector.size() - 1]].endvertex;

			for (int j = first_arc[endvertex]; j < first_arc[endvertex + 1]; j++) // Go through the arcs starting from the current last vertex on the path.
			{
				if (G.arcs[j].endvertex >= i) // We do not include cycles using vertices with number lower than the current starting vertex. These cycles have been added earlier when the starting vertex was lower.
				{
					if (G.arcs[j].endvertex == i) // If the path returns to the intial vertex, save the cycle.
					{
						cycle_arcs temp_cycle;
						temp_cycle.arcs = path_vector;
						temp_cycle.arcs.push_back(j);
						temp_cycle.weight = 0;
						cycles.push_back(temp_cycle);
					}
					else if (distances[i][G.arcs[j].endvertex] + path_vector.size() < config.cyclelength)
					{
						vector<int> new_path = path_vector;
						new_path.push_back(j);
						paths.push(new_path);
					}
				}
			}
		}

	}
	for (int i = 0; i < cycles.size(); i++)
	{
		for (int j = 0; j < cycles[i].arcs.size(); j++)
		{
			cycles[i].vertices.push_back(G.arcs[cycles[i].arcs[j]].startvertex);
			cycles[i].weight = cycles[i].weight + G.arcs[cycles[i].arcs[j]].weight;
		}
	}

	// Before this point, the 'arcs' that are saved are the index in the arc-vector. However, if working with scenarios this index is different from the 'arcnumber'. 
	// Here, we go return to the arcnumber.
	for (int i = 0; i < cycles.size(); i++)
	{
		for (int j = 0; j < cycles[i].arcs.size(); j++)
		{
			cycles[i].arcs[j] = G.arcs[cycles[i].arcs[j]].arcnumber;
		}
	}

	return cycles;
}
