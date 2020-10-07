#include "stdafx.h"
#include "Structures.h"
#include "Functions.h"

void Pre_Test_Float(directedgraph G, const configuration & config);
cycle_variables Generate_Cycle_Var_Float(IloEnv &env, const directedgraph & G, int cyclelength, int nr_scen);
IloNumVarArray Generate_Testvar_Float(IloEnv & env, directedgraph G);
pre_test_result Pre_Test_EE(directedgraph G, configuration & config);

using namespace std;

ILOUSERCUTCALLBACK1(IntegralSolCut, IloExpr, obj_expr) {
	IloNum obj_value = getObjValue();
	IloNum rhs = 0;
	if (obj_value > floor(obj_value))
	{
		rhs = floor(obj_value);
		addLocal(obj_expr <= rhs).end();
	}
}

ILOMIPINFOCALLBACK5(writeincumbentcallback, const configuration&, config, time_t, start_time, const directedgraph&, G, IloNumVarArray, Testvar, IloNum, Lastincumbent)
{
	if (hasIncumbent())
	{
		if (getIncumbentObjValue() - Lastincumbent > 0.01)
		{
			cout << "New Incumbent" << endl;
			Lastincumbent = getIncumbentObjValue();
			pre_test_result results;
			results.objective_value = getIncumbentObjValue() / config.nr_scenarios;
			time_t current_time;
			std::time(&current_time);
			results.computation_time = difftime(current_time, start_time);
			for (int i = 0; i < G.arcs.size(); i++)
			{
				if (getIncumbentValue(Testvar[i]) > 0.99)
				{
					results.tested_arcs.push_back(G.arcs[i]);
				}
			}
			cout << "Off to write to file" << endl;
			Output_Pre_Test(results, config);
		}
	}	
}

void pre_test_main(const configuration & config, directedgraph G)
{
	pre_test_result results;
	if (config.formulation == 1)
		results = HPIEF_Scen(G, config);
	else if (config.formulation == 2)
		results = EE_Scen(G, config);
	else if (config.formulation == 3)
		results = Cycle_Scen(G, config);
	else if (config.formulation == 4)
		results = Unlim_Cycle_Scen(G, config);
	Output_Pre_Test(results, config);
}

pre_test_result HPIEF_Scen(directedgraph G, const configuration & config)
{
	directedgraph Tested_Graph = G;
	vector<directedgraph> Scenarios;
	if (config.failure_type == 1)
	{
		std::cout << "Arcs Fail" << endl;
		if (config.scen_gen == 1)
		{
			Scenarios = Generate_Scenarios_Tight(G, config.nr_scenarios); std::cout << "Tight Scen Generator" << endl;
		}
		else
		{
			Scenarios = Generate_Scenarios(G, config.nr_scenarios); std::cout << "Basic Scen Generator" << endl;
		}

	}
	else if (config.failure_type == 2)
	{
		std::cout << "Vertices Fail" << endl;
		Scenarios = Generate_Scenarios_Vertex_Tight(G, config.nr_scenarios);
	}
	std::cout << "Scenarios Generated" << endl;
	std::cout << Scenarios.size() << endl;


	time_t start_time;
	std::time(&start_time);
	IloEnv env;
	IloModel model(env);
	std::cout << "Generating Variables" << endl;
	vector<vector<vector<IloNumVarArray>>> Cyclevar(config.nr_scenarios); // First position is scenario, second index is the Graph Copy, third the position in the Graph, fourth the individual arcs.
	vector<vector<vector<vector<int>>>> Cyclevar_arc_link(config.nr_scenarios); // A vector to link the variables to the original arc. Cyclevar_arc_link[i][j][k][l] = m, means that this variable corresponds to the m-th arc in the original arc list.
	for(int i = 0; i < config.nr_scenarios; i++)
	{
		cycle_variables cvars = HPIEF_Scen_Generate_Cycle_Var(env, Scenarios[i], config, i);
		Cyclevar[i] = cvars.Cyclevariable;
		Cyclevar_arc_link[i] = cvars.Link_Cyclevar_Arc;
	}
	std::cout << "Cyclevar Generated" << endl;
	vector<vector<IloNumVarArray>> Chainvar(config.nr_scenarios); // First index is the scenario, second index is the position in the graph, third the individual arc.
	vector<vector<vector<int>>> Chainvar_arc_link(config.nr_scenarios); // A vector to link the variables to the original arc.
	for (int i = 0; i < config.nr_scenarios; i++)
	{
		chain_variables cvars = HPIEF_Scen_Generate_Chain_Var(env, Scenarios[i], config, i);
		Chainvar[i] = cvars.Chainvar;
		Chainvar_arc_link[i] = cvars.Link_Chainvar_Arc;
	}
	std::cout << "Chainvar Generated" << endl;
	std::cout << "Variables Generated" << endl;
	// Create the Objective Function
	IloObjective obj = IloMaximize(env);
	IloExpr obj_expr(env);
	for (int scen = 0; scen < config.nr_scenarios; scen++)
	{
		for (int i = 0; i < G.nr_pairs - 1; i++)
		{
			for (int j = 0; j < config.cyclelength; j++)
			{
				if (Cyclevar[scen][i][j].getSize() > 0)
				{
					IloNumArray weights = IloNumArray(env, Cyclevar[scen][i][j].getSize());
					for (int k = 0; k < Cyclevar[scen][i][j].getSize(); k++)
					{
						weights[k] = G.arcs[Cyclevar_arc_link[scen][i][j][k]].weight;
					}
					obj.setLinearCoefs(Cyclevar[scen][i][j], weights);
					obj_expr.setLinearCoefs(Cyclevar[scen][i][j], weights);
				}
			}
		}
		for (int i = 0; i < config.chainlength; i++)
		{
			if (Chainvar[scen][i].getSize() > 0)
			{
				IloNumArray weights = IloNumArray(env, Chainvar[scen][i].getSize());
				for (int k = 0; k < Chainvar[scen][i].getSize(); k++)
				{
					weights[k] = G.arcs[Chainvar_arc_link[scen][i][k]].weight;
				}
				obj.setLinearCoefs(Chainvar[scen][i], weights);
				obj_expr.setLinearCoefs(Chainvar[scen][i], weights);
			}
		}
	}
	model.add(obj);
	std::cout << "Objective Finished" << endl;
	// Create Constraints per scenario.
	vector<IloRangeArray> vertex_inflow_cons(config.nr_scenarios);
	vector<vector<vector<IloRangeArray>>> vertex_flow_cons(config.nr_scenarios);
	vector<vector<IloRangeArray>> vertex_chain_flow_cons(config.nr_scenarios);
	vector<IloRangeArray> NDD_Constraint(config.nr_scenarios);
	for(int scen = 0; scen < config.nr_scenarios; scen++)
	{
		// Max one incoming arc per vertex.
		vertex_inflow_cons[scen] = Build_Vertex_Constraint(env, model, G, Cyclevar[scen], Cyclevar_arc_link[scen], Chainvar[scen], Chainvar_arc_link[scen]);

		// If there is an arc arriving in position i, there should be an outgoing arc in position i+1 in the same copy (excluding the origin vertex in that copy).
		vertex_flow_cons[scen] = Build_Vertex_Flow_Constraint(env, model, G, Cyclevar[scen], Cyclevar_arc_link[scen], config.cyclelength);

		// If there is an outgoing arc_chain in position i, there should be an incoming arc_chain in position [i-1].
		vertex_chain_flow_cons[scen] = Build_Vertex_Flow_Chain_Constraint(env, model, G, Chainvar[scen], Chainvar_arc_link[scen], config.chainlength);

		// At most one outgoing arc for each NDD
		NDD_Constraint[scen] = Build_NDD_Constraint(env, model, G, Chainvar[scen], Chainvar_arc_link[scen]);
	}
	cout << "Scenario Constraints Finished" << endl;
	// Create Testing Variables and Constraints.
	IloNumVarArray Testvar = Generate_Testvar(env, G);
	vector<IloRangeArray> test_constraint = Build_Test_Constraint(env, model, G, Testvar, Cyclevar, Cyclevar_arc_link, Chainvar, Chainvar_arc_link, config.nr_scenarios);
	IloRange Max_Test_Constraint = Build_Max_Test_Constraint(env, model, Testvar, config.max_test);
	
	if (config.bender_version == 1)
	{
		Generate_Testvar_Connecting_Constraints(env, model, G, Testvar);
	}
	else if (config.bender_version == 2)
	{
		cout << "New Strong Bender Constraints" << endl;
		chain_variables Testchain = Generate_Chain_Var(env, G, config.chainlength);
		cycle_variables Testcycle = Generate_Cycle_Var(env, G, config.cyclelength);
		cout << "Vars generated for Bender Constraints" << endl;
		Generate_Testvar_Constraints(env, model, G, config, Testvar, Testcycle, Testchain);
	}
	else if (config.bender_version == 3)
	{
		cout << "Old Strong Bender Constraints" << endl;
		vector<vector<IloNumVarArray>> Testcycle = Generate_TestCycle_Var(env, G, config);
		Generate_Testvar_Cycle_Constraints(env, model, G, config, Testvar, Testcycle);
	}
	cout << "Bender Constraints Finished" << endl;

	// Test for Benders
	/*IloRangeArray scen_constraint(env, config.nr_scenarios);
	if (config.solver == 5)
	{
		for (int scen = 0; scen < config.nr_scenarios; scen++)
		{
			IloExpr expr(env);
			scen_constraint[scen] = IloRange(expr <= 0);
			for (int i = 0; i < Testvar.getSize(); i++)
				scen_constraint[scen].setLinearCoef(Testvar[i], -1);
			for (int i = 0; i < G.nr_pairs - 1; i++)
			{
				for (int j = 0; j < config.cyclelength; j++)
				{
					for (int k = 0; k < Cyclevar[scen][i][j].getSize(); k++)
					{
						scen_constraint[scen].setLinearCoef(Cyclevar[scen][i][j][k], G.arcs[Cyclevar_arc_link[scen][i][j][k]].weight);
					}
				}
			}
		}
		model.add(scen_constraint);
	}*/

	IloCplex CPLEX(model);
	CPLEX.setParam(IloCplex::Param::TimeLimit, config.time_limit);
	//CPLEX.exportModel("model.lp");
	if(config.solver == 4)
		CPLEX.setParam(IloCplex::Param::Benders::Strategy, IloCplex::BendersFull);

	//std::cout << "Adding to model" << endl;
	//IloNum rhs = 10000;
	//model.add(obj_expr <= rhs);
	CPLEX.exportModel("00test.lp");
	//if (config.solver == 3 /*|| config.solver == 4*/)
	//{
		//CPLEX.use(IntegralSolCut(env, obj_expr)); // Does not appear to work in combination with BendersFull strategy.
		//CPLEX.setParam(IloCplex::ParallelMode, -1);
		//CPLEX.setParam(IloCplex::Threads, 8);
	//}
	std:: cout << "Adding callback" << endl;
	IloNum Incumbentvalue = 0;
	CPLEX.use(writeincumbentcallback(env, config, start_time, G, Testvar, Incumbentvalue));
	std::cout << "Added callback" << endl;

	std::cout << "Added to model" << endl;
	CPLEX.solve();
	std::cout << "Solved model" << endl;
	pre_test_result results;
	results.objective_value = CPLEX.getObjValue() / config.nr_scenarios;
	std::cout << results.objective_value << endl;

	time_t current_time;
	std::time(&current_time);
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

vector<directedgraph> Generate_Scenarios(const directedgraph & G, int nr_scen)
{
	//ofstream output;
	//output.open("Scenarios.txt");

	//srand(time(NULL));

	vector<directedgraph> Scenarios;
	for (int i = 0; i < nr_scen; i++)
	{
		//output << "Scenario " << i << endl;
		Scenarios.push_back(G);
		Scenarios[i].arcs.resize(0); // Empty out the arcs.
		for (int j = 0; j < G.arcs.size(); j++)
		{
			if (rand() % 100 >= Scenarios[i].arcs[j].failprob*100)
			{
				Scenarios[i].arcs.push_back(G.arcs[j]);
				//output << "(" << G.arcs[j].startvertex << "," << G.arcs[j].endvertex << ")" << endl;
			}
		}
		//output << endl;
	}
	return Scenarios;
}

vector<directedgraph> Generate_Scenarios_Tight(const directedgraph & G, int nr_scen)
{
	ofstream output;
	//srand(time(NULL));
	vector<directedgraph> Scenarios(nr_scen);
	// Get the number of successes for each arc and the number of scenarios.
	vector<int> succes_per_arc(G.arcs.size());
	for (int i = 0; i < G.arcs.size(); i++)
	{
		float expected_succes = nr_scen*(1 - G.arcs[i].failprob);
		float natural;
		float remainder = modf(expected_succes, & natural);
		// Round it to an integer probabilistically.
		if (rand() % 1000 < remainder * 1000)
			remainder = 1;
		else
			remainder = 0;
		succes_per_arc[i] = natural + remainder;
	}
	for (int i = 0; i < nr_scen; i++)
	{
		Scenarios[i] = G;
		Scenarios[i].arcs.resize(0); // Empty out the arcs.
		for (int j = 0; j < G.arcs.size(); j++)
		{
			if (rand() % (nr_scen - i) < succes_per_arc[j])
			{
				succes_per_arc[j]--;
				Scenarios[i].arcs.push_back(G.arcs[j]);
			}
		}
	}
	return Scenarios;
}

vector<directedgraph> Generate_Scenarios_Vertex_Tight(const directedgraph & G, int nr_scen)
{
	ofstream output;
	//srand(time(NULL));
	vector<directedgraph> Scenarios(nr_scen);
	// Get the number of successes for each vertex and the number of scenarios.
	vector<int> succes_per_vertex(G.size);
	for (int i = 0; i < G.size; i++)
	{
		float expected_succes;
		if (i < G.nr_pairs)
		{
			expected_succes = nr_scen*(1 - G.pairs[i].failprob);
		}
		else
			expected_succes = nr_scen*(1 - G.ndds[i - G.nr_pairs].failprob);
		float natural;
		float remainder = modf(expected_succes, &natural);
		// Round it to an integer probabilistically.
		if (rand() % 1000 < remainder * 1000)
			remainder = 1;
		else
			remainder = 0;
		succes_per_vertex[i] = natural + remainder;
	}
	for (int i = 0; i < nr_scen; i++)
	{
		Scenarios[i] = G;
		Scenarios[i].arcs.resize(0); // Empty out the arcs.
		vector<bool> vertex_succes(G.size);
		for (int j = 0; j < G.size; j++)
		{
			if (rand() % (nr_scen - i) < succes_per_vertex[j])
			{
				succes_per_vertex[j]--;
				vertex_succes[j] = 1;
			}
		}
		for (int j = 0; j < G.arcs.size(); j++)
		{
			if (vertex_succes[G.arcs[j].startvertex] == 1 && vertex_succes[G.arcs[j].endvertex] == 1)
			{
				Scenarios[i].arcs.push_back(G.arcs[j]);
			}
		}
	}
	return Scenarios;
}

vector<directedgraph> Generate_Scenarios_Vertex(const directedgraph & G, int nr_scen)
{
	ofstream output;
	//srand(time(NULL));
	vector<directedgraph> Scenarios(nr_scen);
	// Get the number of successes for each vertex and the number of scenarios.
	for (int i = 0; i < nr_scen; i++)
	{
		Scenarios[i] = G;
		Scenarios[i].arcs.resize(0); // Empty out the arcs.
		vector<bool> vertex_succes(G.size);
		for (int j = 0; j < G.size; j++)
		{
			if (j < G.nr_pairs)
			{
				if (rand() % 100 > Scenarios[i].pairs[j].failprob * 100)
				{
					vertex_succes[j] = 1;
				}
			}
			else
			{
				if (rand() % 100 > Scenarios[i].ndds[j - G.nr_pairs].failprob * 100)
					vertex_succes[j] = 1;
			}
			
		}
		for (int j = 0; j < G.arcs.size(); j++)
		{
			if (vertex_succes[G.arcs[j].startvertex] == 1 && vertex_succes[G.arcs[j].endvertex] == 1)
			{
				Scenarios[i].arcs.push_back(G.arcs[j]);
			}
		}
	}
	return Scenarios;
}

cycle_variables HPIEF_Scen_Generate_Cycle_Var(IloEnv &env, const directedgraph & G, const configuration & config, int nr_scen)
{
	// Note that we consistently work with nr_pairs - 1. Since the copy corresponding to the last pair is empty (no arcs), it is useless to include it.
	cycle_variables c;
	c.Cyclevariable.resize(G.nr_pairs - 1);
	c.Link_Cyclevar_Arc.resize(G.nr_pairs - 1);
	// Pre-Processing
	vector<vector<directedarc>> Acopies = DP_Copy(G);
	vector<vector<vector<int>>> copy_pos_arc_possible = cycle_preproces(G, Acopies, config.cyclelength);
	// Create all variables
	{
		ostringstream convert;
		for (int i = 0; i < G.nr_pairs - 1; i++)
		{
			c.Link_Cyclevar_Arc[i].resize(config.cyclelength);
			for (int j = 0; j < config.cyclelength; j++)
			{
				c.Cyclevariable[i].push_back(IloNumVarArray(env));
				for (int k = 0; k < Acopies[i].size(); k++)
				{
					if (copy_pos_arc_possible[i][j][k] == 1)
					{
						convert << "x(" << nr_scen << "," << Acopies[i][k].startvertex << "," << Acopies[i][k].endvertex << "," << i << "," << j << ")";
						string varname = convert.str();
						const char* vname = varname.c_str();
						if(config.solver == 3 || config.solver == 4)
							c.Cyclevariable[i][j].add(IloNumVar(env, 0, 1, ILOFLOAT, vname));
						else
							c.Cyclevariable[i][j].add(IloNumVar(env, 0, 1, ILOINT, vname));
						c.Link_Cyclevar_Arc[i][j].push_back(Acopies[i][k].arcnumber);
						convert.str("");
						convert.clear();
					}
				}
			}
		}
	}
	return c;
}

chain_variables HPIEF_Scen_Generate_Chain_Var(IloEnv & env, directedgraph G, const configuration & config, int nr_scen)
{
	chain_variables c;
	c.Link_Chainvar_Arc.resize(G.nr_pairs - 1);

	// Pre-Processing. For each arc of G, the following function checks whether they can be in a particular position in any feasible solution.
	vector<vector<int>> arc_position_possible = chain_preproces(G, config.chainlength);
	// The first index is Position, the second is the individual arc.

	// Create all variables
	for (int i = 0; i < config.chainlength; i++)
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
					if(config.solver == 3 || config.solver == 4)
						c.Chainvar[i].add(IloNumVar(env, 0, 1, ILOFLOAT, vname));
					else
						c.Chainvar[i].add(IloNumVar(env, 0, 1, ILOINT, vname));
					c.Link_Chainvar_Arc[i].push_back(G.arcs[j].arcnumber);
				}
				else if (i > 0 && G.arcs[j].startvertex < G.nr_pairs) // Any arc between pairs can be used in the following positions
				{
					ostringstream convert;
					convert << "y(" << G.arcs[j].startvertex << "," << G.arcs[j].endvertex << "," << i << ")";
					string varname = convert.str();
					const char* vname = varname.c_str();
					if (config.solver == 3 || config.solver == 4)
						c.Chainvar[i].add(IloNumVar(env, 0, 1, ILOFLOAT, vname));
					else
						c.Chainvar[i].add(IloNumVar(env, 0, 1, ILOINT, vname));
					c.Link_Chainvar_Arc[i].push_back(G.arcs[j].arcnumber);
				}
			}
		}
	}

	return c;
}

IloNumVarArray Generate_Testvar(IloEnv & env, directedgraph G)
{
	IloNumVarArray Testvar(env);
	for (int i = 0; i < G.arcs.size(); i++)
	{
		ostringstream convert;
		convert << "t("  << G.arcs[i].startvertex << "," << G.arcs[i].endvertex << ")";
		string varname = convert.str();
		const char* vname = varname.c_str();
		Testvar.add(IloNumVar(env, 0, 1, ILOINT, vname));
	}

	return Testvar;
}

vector<vector<IloNumVarArray>> Generate_TestCycle_Var(IloEnv & env, const directedgraph & G, const configuration & config)
{
	vector<vector<IloNumVarArray>> TestCycle_Var(G.size);
	for (int copy = 0; copy < G.size; copy++)
	{
		TestCycle_Var[copy].resize(config.cyclelength);
		for (int pos = 0; pos < config.cyclelength; pos++)
		{
			// If the position is not the final position, all arcs can be used for this flow (in theory, position 1 is only for arcs going out
			// of the original vertex, but there is no reason to set this to one for any other arc, we allow it, CPLEX simply won't use it.)
			if (pos < config.cyclelength - 1)
			{
				TestCycle_Var[copy][pos] = IloNumVarArray(env, G.arcs.size(), 0, 1, ILOINT);
				for (int arc = 0; arc < G.arcs.size(); arc++)
				{
					ostringstream convert;
					convert << "TestCyle[" << copy << "," << pos << ",(" << G.arcs[arc].startvertex << "," << G.arcs[arc].endvertex << ")]";
					string varname = convert.str();
					const char* vname = varname.c_str();
					TestCycle_Var[copy][pos][arc].setName(vname);
				}
			}
			// We only use the final position for arcs going back to the origin vertex.
			else
			{
				TestCycle_Var[copy][pos] = IloNumVarArray(env, G.arcs.size(), 0, 0, ILOINT);
				for (int arc = 0; arc < G.arcs.size(); arc++)
				{
					ostringstream convert;
					convert << "TestCyle[" << copy << "," << pos << ",(" << G.arcs[arc].startvertex << "," << G.arcs[arc].endvertex << ")]";
					string varname = convert.str();
					const char* vname = varname.c_str();
					TestCycle_Var[copy][pos][arc].setName(vname);
					if (G.arcs[arc].endvertex == copy)
						TestCycle_Var[copy][pos][arc].setUB(1);
				}
			}
		}
	}
	return TestCycle_Var;
}

vector<IloRangeArray> Build_Test_Constraint(IloEnv & env, IloModel model, directedgraph G, const IloNumVarArray & Testvar, const vector<vector<vector<IloNumVarArray>>>& Cyclevar, const vector<vector<vector<vector<int>>>>& cycle_link, const vector<vector<IloNumVarArray>>& Chainvar, const vector<vector<vector<int>>>& chain_link, int nr_scen)
{
	vector<IloRangeArray> Test_Constraints(nr_scen);

	for (int scen = 0; scen < nr_scen; scen++)
	{
		Test_Constraints[scen] = IloRangeArray(env, G.arcs.size());
		for (int i = 0; i < G.arcs.size(); i++)
		{
			ostringstream convert;
			convert << "Test(S:" << scen << ",(" << G.arcs[i].startvertex << "," << G.arcs[i].endvertex << "))";
			string varname = convert.str();
			const char* vname = varname.c_str();
			Test_Constraints[scen][i] = IloRange(-Testvar[i] <= 0);
			Test_Constraints[scen][i].setName(vname);
		}
		for (int i = 0; i < Cyclevar[scen].size(); i++) // Scen
		{
			for (int j = 0; j < Cyclevar[scen][i].size(); j++) // Copy
			{
				for (int k = 0; k < Cyclevar[scen][i][j].getSize(); k++) // Position
				{
					Test_Constraints[scen][cycle_link[scen][i][j][k]].setLinearCoef(Cyclevar[scen][i][j][k],1);
				}
			}
		}
		for (int i = 0; i < Chainvar[scen].size(); i++) // Scen
		{
			for (int j = 0; j < Chainvar[scen][i].getSize(); j++) // Position
			{

				Test_Constraints[scen][chain_link[scen][i][j]].setLinearCoef(Chainvar[scen][i][j], 1);
			}
		}
		model.add(Test_Constraints[scen]);
	}

	return Test_Constraints;
}

IloRange Build_Max_Test_Constraint(IloEnv & env, IloModel model, const IloNumVarArray & Testvar, int max_test)
{
	IloExpr expr(env);
	IloRange Constraint(expr <= max_test);
	for (int i = 0; i < Testvar.getSize(); i++)
		Constraint.setLinearCoef(Testvar[i], 1);

	model.add(Constraint);
	return Constraint;
}

void Output_Pre_Test(const pre_test_result & results, const configuration & config)
{
	ofstream output;
	output.open(config.testvar_output);
	output << "Nr_Pairs = " << config.nr_pairs << endl;
	output << "Nr_NDD = " << config.nr_ndd << endl;

	for (int i = 0; i < results.tested_arcs.size(); i++)
	{
		output << "(" << results.tested_arcs[i].startvertex << "," << results.tested_arcs[i].endvertex << "), " << results.tested_arcs[i].failprob << ", " << results.tested_arcs[i].weight << endl;
	}

	output << "Objective Value = " << results.objective_value << endl;
	output << "Computation Time = " << results.computation_time << endl;
	output << "Nr Scenarios = " << config.nr_scenarios << endl;
	output << "Nr Test = " << config.max_test << endl;
}

IloNumVarArray Generate_Testvar_Float(IloEnv & env, directedgraph G)
{
	IloNumVarArray Testvar(env);
	for (int i = 0; i < G.arcs.size(); i++)
	{
		ostringstream convert;
		convert << "t(" << G.arcs[i].startvertex << "," << G.arcs[i].endvertex << ")";
		string varname = convert.str();
		const char* vname = varname.c_str();
		Testvar.add(IloNumVar(env, 0, 1, ILOFLOAT, vname));
	}

	return Testvar;
}

cycle_variables Generate_Cycle_Var_Float(IloEnv &env, const directedgraph & G, int cyclelength, int nr_scen)
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
						ostringstream convert;
						convert << "x(" << nr_scen << "," << Acopies[i][k].startvertex << "," << Acopies[i][k].endvertex << "," << i << "," << j << ")";
						string varname = convert.str();
						const char* vname = varname.c_str();
						c.Cyclevariable[i][j].add(IloNumVar(env, 0, 1, ILOFLOAT, vname));
						c.Link_Cyclevar_Arc[i][j].push_back(Acopies[i][k].arcnumber);
						convert.str("");
						convert.clear();
					}
				}
			}
		}
	}
	return c;
}

void Pre_Test_Float(directedgraph G, const configuration & config)
{
	directedgraph Tested_Graph = G;
	vector<directedgraph> Scenarios;
	if (config.failure_type == 1)
	{
		std::cout << "Arcs Fail" << endl;
		if (config.scen_gen == 1)
		{
			Scenarios = Generate_Scenarios_Tight(G, config.nr_scenarios); std::cout << "Tight Scen Generator" << endl;
		}
		else
		{
			Scenarios = Generate_Scenarios(G, config.nr_scenarios); std::cout << "Basic Scen Generator" << endl;
		}

	}
	else if (config.failure_type == 2)
	{
		std::cout << "Vertices Fail" << endl;
		Scenarios = Generate_Scenarios_Vertex_Tight(G, config.nr_scenarios);
	}
	std::cout << "Scenarios Generated" << endl;
	std::cout << Scenarios.size();


	time_t start_time;
	std::time(&start_time);
	IloEnv env;
	IloModel model(env);
	std::cout << "Generating Variables" << endl;
	vector<vector<vector<IloNumVarArray>>> Cyclevar(config.nr_scenarios); // First position is scenario, second index is the Graph Copy, third the position in the Graph, fourth the individual arcs.
	vector<vector<vector<vector<int>>>> Cyclevar_arc_link(config.nr_scenarios); // A vector to link the variables to the original arc. Cyclevar_arc_link[i][j][k][l] = m, means that this variable corresponds to the m-th arc in the original arc list.
	for (int i = 0; i < config.nr_scenarios; i++)
	{
		cycle_variables cvars = Generate_Cycle_Var_Float(env, Scenarios[i], config.cyclelength, i);
		Cyclevar[i] = cvars.Cyclevariable;
		Cyclevar_arc_link[i] = cvars.Link_Cyclevar_Arc;
	}
	std::cout << "Cyclevar Generated" << endl;
	vector<vector<IloNumVarArray>> Chainvar(config.nr_scenarios); // First index is the scenario, second index is the position in the graph, third the individual arc.
	vector<vector<vector<int>>> Chainvar_arc_link(config.nr_scenarios); // A vector to link the variables to the original arc.
	for (int i = 0; i < config.nr_scenarios; i++)
	{
		chain_variables cvars = HPIEF_Scen_Generate_Chain_Var(env, Scenarios[i], config, i);
		Chainvar[i] = cvars.Chainvar;
		Chainvar_arc_link[i] = cvars.Link_Chainvar_Arc;
	}
	std::cout << "Chainvar Generated" << endl;
	std::cout << "Variables Generated" << endl;
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

	// Create Constraints per scenario.
	vector<IloRangeArray> vertex_inflow_cons(config.nr_scenarios);
	vector<vector<vector<IloRangeArray>>> vertex_flow_cons(config.nr_scenarios);
	vector<vector<IloRangeArray>> vertex_chain_flow_cons(config.nr_scenarios);
	vector<IloRangeArray> NDD_Constraint(config.nr_scenarios);
	for (int scen = 0; scen < config.nr_scenarios; scen++)
	{
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
	IloNumVarArray Testvar = Generate_Testvar_Float(env, G);
	vector<IloRangeArray> test_constraint = Build_Test_Constraint(env, model, G, Testvar, Cyclevar, Cyclevar_arc_link, Chainvar, Chainvar_arc_link, config.nr_scenarios);
	IloRange Max_Test_Constraint = Build_Max_Test_Constraint(env, model, Testvar, config.max_test);

	IloCplex CPLEX(model);
	CPLEX.exportModel("Floatmodel.lp");
	CPLEX.setParam(IloCplex::TiLim, config.time_limit);
	CPLEX.solve();

	pre_test_result results;
	results.objective_value = CPLEX.getObjValue() / config.nr_scenarios;
	std::cout << results.objective_value << endl;

	time_t current_time;
	std::time(&current_time);
	results.computation_time = difftime(current_time, start_time);

	ofstream output;
	output.open("Float_LP");
	output << "Nr_Pairs = " << G.nr_pairs << endl;
	output << "Nr_NDD = " << G.nr_ndd << endl;

	for (int i = 0; i < Testvar.getSize(); i++)
	{
		if(CPLEX.getValue(Testvar[i]) > 0)
		output << "(" << G.arcs[i].startvertex << "," << G.arcs[i].endvertex << "), " << CPLEX.getValue(Testvar[i]) << endl;
	}

	output << "Objective Value = " << results.objective_value << endl;
	output << "Computation Time = " << results.computation_time << endl;
	output << "Nr Test = " << config.max_test << endl;
	cin.get();
}

pre_test_result Pre_Test_EE(directedgraph G, configuration &config)
{
	directedgraph Tested_Graph = G;
	vector<directedgraph> Scenarios;
	if (config.failure_type == 1)
	{
		std::cout << "Arcs Fail" << endl;
		if (config.scen_gen == 1)
		{
			Scenarios = Generate_Scenarios_Tight(G, config.nr_scenarios); std::cout << "Tight Scen Generator" << endl;
		}
		else
		{
			Scenarios = Generate_Scenarios(G, config.nr_scenarios); std::cout << "Basic Scen Generator" << endl;
		}

	}
	else if (config.failure_type == 2)
	{
		std::cout << "Vertices Fail" << endl;
		Scenarios = Generate_Scenarios_Vertex_Tight(G, config.nr_scenarios);
	}
	std::cout << "Scenarios Generated" << endl;

	time_t start_time;
	std::time(&start_time);
	IloEnv env;
	IloModel model(env);
	std::cout << "Generating Variables" << endl;
	vector<vector<IloNumVarArray>> Cyclevar(config.nr_scenarios); // First position is scenario, second index is the Graph Copy, third the individual arcs.
	vector<vector<vector<int>>> Cyclevar_arc_link(config.nr_scenarios); // A vector to link the variables to the original arc. Cyclevar_arc_link[i][j][k] = m, means that this variable corresponds to the m-th arc in the original arc list.
	for (int i = 0; i < config.nr_scenarios; i++)
	{
		cycle_variables_EE cvars = Generate_Cycle_Var_EE(env, Scenarios[i], config.cyclelength, i);
		Cyclevar[i] = cvars.Cycle_Arc_Variable;
		Cyclevar_arc_link[i] = cvars.Link_Variable_arc;
	}
	std::cout << "Cyclevar Generated" << endl;
	// Include chainvar if needed
	std::cout << "Variables Generated" << endl;
	// Create the Objective Function
	IloObjective obj = IloMaximize(env);
	for (int scen = 0; scen < config.nr_scenarios; scen++)
	{
		for (int copy = 0; copy < G.nr_pairs - 1; copy++)
		{
			if (Cyclevar[scen][copy].getSize() > 0)
			{
				IloNumArray weights(env, Cyclevar[scen][copy].getSize());
				for (int arc = 0; arc < Cyclevar[scen][copy].getSize(); arc++)
				{
					weights[arc] = G.arcs[Cyclevar_arc_link[scen][copy][arc]].weight;
				}
				obj.setLinearCoefs(Cyclevar[scen][copy], weights);
			}
		}
	}
	model.add(obj);
	std::cout << "Objective Created" << endl;

	// Create Constraints per scenario.
	vector<IloRangeArray> vertex_inflow_cons_EE(config.nr_scenarios);
	vector<vector<IloRangeArray>> vertex_flow_cons_EE(config.nr_scenarios);
	vector<IloRangeArray> sym_length_con(config.nr_scenarios);

	for (int scen = 0; scen < config.nr_scenarios; scen++)
	{
		if (scen % 100 == 0)
			std::cout << "Generating constraints for scenario " << scen << endl;
		// Max one incoming arc per vertex.
		vertex_inflow_cons_EE[scen] = Build_Vertex_Constraint_EE(env, model, G, Cyclevar[scen], Cyclevar_arc_link[scen]);

		// If there is an arc arriving in position i, there should be an outgoing arc in position i+1 in the same copy (excluding the origin vertex in that copy).
		vertex_flow_cons_EE[scen] = Build_Vertex_Flow_Constraint_EE(env, model, G, Cyclevar[scen], Cyclevar_arc_link[scen]);

		// Combined symmetry and cycle length constraint.
		sym_length_con[scen] = Build_Symmetry_Cycle_Length_Constraint_EE(env, model, G, Cyclevar[scen], Cyclevar_arc_link[scen], config.cyclelength);
	}

	//Create Testing Variables and Constraints.
	IloNumVarArray Testvar = Generate_Testvar(env, G);
	vector<IloRangeArray> test_constraint = Build_Test_Constraint_EE(env, model, G, Testvar, Cyclevar, Cyclevar_arc_link, config.nr_scenarios);
	IloRange Max_Test_Constraint = Build_Max_Test_Constraint(env, model, Testvar, config.max_test);

	IloCplex CPLEX(model);
	CPLEX.setParam(IloCplex::TiLim, config.time_limit);
	CPLEX.setParam(IloCplex::TreLim, config.memory_limit);
	if (config.solver == 4)
		CPLEX.setParam(IloCplex::Param::Benders::Strategy, IloCplex::BendersFull);
	CPLEX.solve();

	pre_test_result results;
	results.objective_value = CPLEX.getObjValue() / config.nr_scenarios;
	std::cout << results.objective_value << endl;

	time_t current_time;
	std::time(&current_time);
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

cycle_variables_EE Generate_Cycle_Var_EE(IloEnv &env, directedgraph G, int cyclelength, int scenario_number)
{
	// Note that we consistently work with nr_pairs - 1, since the copy corresonding to the last pair can not contain any cycles.
	cycle_variables_EE c;
	c.Cycle_Arc_Variable.resize(G.nr_pairs - 1);
	c.Link_Variable_arc.resize(G.nr_pairs - 1);
	// Pre-Processing
	vector<vector<directedarc>> Acopies = DP_Copy(G);
	vector<vector<int>> copy_arc_possible = cycle_preproces_EE(G, Acopies, cyclelength); // First index is graph copy, second is arc.
	vector<int> arc_per_copy(G.nr_pairs - 1, 0);
	for (int i = 0; i < copy_arc_possible.size(); i++)
	{
		for (int j = 0; j < copy_arc_possible[i].size(); j++)
		{
			if (copy_arc_possible[i][j] == 1)
				arc_per_copy[i]++;
		}
	}

	// Create all variables
	for (int i = 0; i < G.nr_pairs - 1; i++)
	{
		c.Link_Variable_arc[i].resize(arc_per_copy[i]);
		c.Cycle_Arc_Variable[i] = IloNumVarArray(env, arc_per_copy[i]);
		int vars_added = 0;
		for (int arc = 0; arc < Acopies[i].size(); arc++)
		{
			if (copy_arc_possible[i][arc] == 1)
			{
				ostringstream convert;
				convert << "x(" << Acopies[i][arc].startvertex << "," << Acopies[i][arc].endvertex << "," << scenario_number << ")";
				string varname = convert.str();
				const char* vname = varname.c_str();
				c.Cycle_Arc_Variable[i][vars_added] = IloNumVar(env, 0, 1, ILOINT, vname);
				c.Link_Variable_arc[i][vars_added] = Acopies[i][arc].arcnumber;
				vars_added++;
			}

		}
	}
	return c;
}

IloRangeArray Generate_Testvar_Flow_Constraints(IloEnv & env, const directedgraph & G, const IloNumVarArray & Testvar) {
	IloRangeArray Testvar_Flow_Constraints(env, G.nr_pairs);
	for (unsigned int i = 0; i < G.nr_pairs; ++i) {
		ostringstream convert;
		convert << "TestvarFlow(" << i << ")";
		string varname = convert.str();
		const char* vname = varname.c_str();
		Testvar_Flow_Constraints[i] = IloRange(env, 0, 0, vname);
		for (unsigned int k = 0; k < G.arcs.size(); ++k) {
			if (G.arcs[k].startvertex == i) Testvar_Flow_Constraints[i].setLinearCoef(Testvar[k], -1);
			if (G.arcs[k].endvertex == i) Testvar_Flow_Constraints[i].setLinearCoef(Testvar[k], 1);
		}
	}
	return Testvar_Flow_Constraints;
}

IloRangeArray Generate_Testvar_Connecting_Constraints(IloEnv & env, IloModel & model, const directedgraph & G, const IloNumVarArray & Testvar) {
	IloRangeArray Testvar_OutConnecting_Constraints(env, G.arcs.size());
	IloRangeArray Testvar_InConnecting_Constraints(env, G.arcs.size());
	for (unsigned int i = 0; i < G.arcs.size(); ++i) {
		IloExpr expr_out(env);
		IloExpr expr_in(env);
		Testvar_OutConnecting_Constraints[i] = IloRange(expr_out <= 0);
		Testvar_InConnecting_Constraints[i] = IloRange(expr_in <= 0);
		Testvar_OutConnecting_Constraints[i].setUB(0);
		Testvar_InConnecting_Constraints[i].setUB(0);
		Testvar_OutConnecting_Constraints[i].setLinearCoef(Testvar[i], 1);
		Testvar_InConnecting_Constraints[i].setLinearCoef(Testvar[i], 1);
		for (unsigned int k = 0; k < G.arcs.size(); ++k) {
			if (G.arcs[k].startvertex == G.arcs[i].endvertex) Testvar_OutConnecting_Constraints[i].setLinearCoef(Testvar[k], -1);
			if (G.arcs[k].endvertex == G.arcs[i].startvertex) Testvar_InConnecting_Constraints[i].setLinearCoef(Testvar[k], -1);
		}
	}
	model.add(Testvar_OutConnecting_Constraints);
	model.add(Testvar_InConnecting_Constraints);
	std::cout << "Added Testvar Connecting Constraints" << endl;
}

IloRangeArray Generate_Testvar_Cycle_Constraints(IloEnv & env, IloModel & model, const directedgraph & G, const configuration & config, const IloNumVarArray & Testvar, const vector<vector<IloNumVarArray>>& TestCycle_Var)
{
	IloRangeArray First_Position_Constraint(env, G.arcs.size());
	for (int arc = 0; arc < G.arcs.size(); arc++)
	{
		IloExpr expr(env);
		First_Position_Constraint[arc] = IloRange(expr <= 0);
		First_Position_Constraint[arc].setLinearCoef(Testvar[arc], 1);
		First_Position_Constraint[arc].setLinearCoef(TestCycle_Var[G.arcs[arc].startvertex][0][arc], -1);
	}
	model.add(First_Position_Constraint);

	vector<vector<IloRangeArray>> Test_Use_Constraint(G.size);
	for (int copy = 0; copy < G.size; copy++)
	{
		Test_Use_Constraint[copy].resize(config.cyclelength - 1); // No constraints for position 1.
		for (int pos = 0; pos < config.cyclelength - 1; pos++)
		{
			Test_Use_Constraint[copy][pos] = IloRangeArray(env, G.arcs.size());
			for (int arc = 0; arc < G.arcs.size(); arc++)
			{
				// Pos + 1, because this constraint is not necessary for position 0, only for 1 to K-1
				Test_Use_Constraint[copy][pos][arc] = IloRange(TestCycle_Var[copy][pos + 1][arc] - Testvar[arc] <= 0);
			}
			model.add(Test_Use_Constraint[copy][pos]);
		}
	}

	vector<vector<IloRangeArray>> Test_Connection_Constraint(G.size);
	for (int copy = 0; copy < G.size; copy++)
	{
		Test_Connection_Constraint[copy].resize(config.cyclelength - 1);
		for (int pos = 0; pos < config.cyclelength - 1; pos++)
		{
			Test_Connection_Constraint[copy][pos] = IloRangeArray(env, G.arcs.size());
			for (int arc = 0; arc < G.arcs.size(); arc++)
			{
				if (copy != G.arcs[arc].endvertex)
				{
					Test_Connection_Constraint[copy][pos][arc] = IloRange(TestCycle_Var[copy][pos][arc] <= 0);
					for (int arc2 = 0; arc2 < G.arcs.size(); arc2++)
					{
						if (G.arcs[arc].endvertex == G.arcs[arc2].startvertex)
						{
							Test_Connection_Constraint[copy][pos][arc].setLinearCoef(TestCycle_Var[copy][pos + 1][arc2], -1);
						}
					}
				}
			}
			model.add(Test_Connection_Constraint[copy][pos]);
		}
	}
	std::cout << "*Generating Cycle <= K Constraints " << endl;

}

void Generate_Testvar_Constraints(IloEnv & env, IloModel & model, const directedgraph & G, const configuration & config, const IloNumVarArray & Testvar, const cycle_variables & TestCycle_Var, const chain_variables & TestChain_Var)
{
	// The first constraints require that each tested arc are part of at least one cycle or chain.
	IloRangeArray Test_Use_Constraint(env, G.arcs.size());
	for (int arc = 0; arc < G.arcs.size(); arc++) // We first set up that each arc can not be tested unless we have another variable with negative coefficient in its constraint.
	{
		ostringstream convert;
		convert << "Test_Use_Constraint:" << "(" << G.arcs[arc].startvertex << "," << G.arcs[arc].endvertex << "))";
		string varname = convert.str();
		const char* vname = varname.c_str();
		Test_Use_Constraint[arc] = IloRange(Testvar[arc] <= 0);
		Test_Use_Constraint[arc].setName(vname);
	}
	// Add all the variables corresponding to cycles to the constraints.
	for (int copy = 0; copy < G.nr_pairs - 1; copy++)
	{
		for (int position = 0; position < config.cyclelength; position++)
		{
			for (int arc = 0; arc < TestCycle_Var.Cyclevariable[copy][position].getSize(); arc++)
			{
				Test_Use_Constraint[TestCycle_Var.Link_Cyclevar_Arc[copy][position][arc]].setLinearCoef(TestCycle_Var.Cyclevariable[copy][position][arc], -1);
			}
		}
	}
	// Add all the variables corresponding to chains to the constraints.
	for (int position = 0; position < config.chainlength; position++)
	{
		for (int arc = 0; arc < TestChain_Var.Chainvar[position].getSize(); arc++)
		{
			Test_Use_Constraint[TestChain_Var.Link_Chainvar_Arc[position][arc]].setLinearCoef(TestChain_Var.Chainvar[position][arc], -1);
		}
	}
	model.add(Test_Use_Constraint);
	cout << "First Bender Constraint Set" << endl;
	// The second constraints enforce that a cycle (or chain) variable is only usable if the corresponding arc is tested.
	vector<vector<IloRangeArray>> Use_Test_Constraint_Cycle(G.nr_pairs - 1); // (nr_pairs - 1) as the first index i the graph copy.
	for (int copy = 0; copy < G.nr_pairs - 1; copy++)
	{
		Use_Test_Constraint_Cycle[copy].resize(config.cyclelength);
		for (int position = 0; position < config.cyclelength; position++)
		{
			Use_Test_Constraint_Cycle[copy][position] = IloRangeArray(env, TestCycle_Var.Cyclevariable[copy][position].getSize());
			for (int arc = 0; arc < TestCycle_Var.Cyclevariable[copy][position].getSize(); arc++)
			{
				Use_Test_Constraint_Cycle[copy][position][arc] = IloRange(TestCycle_Var.Cyclevariable[copy][position][arc] <= 0);
				Use_Test_Constraint_Cycle[copy][position][arc].setLinearCoef(Testvar[TestCycle_Var.Link_Cyclevar_Arc[copy][position][arc]], -1);
			}
			model.add(Use_Test_Constraint_Cycle[copy][position]);
		}
	}
	vector<IloRangeArray> Use_Test_Constraint_Chain(config.chainlength); // The first index is the position here.
	for (int position = 0; position < config.chainlength; position++)
	{
		Use_Test_Constraint_Chain[position] = IloRangeArray(env, TestChain_Var.Chainvar[position].getSize());
		for (int arc = 0; arc < TestChain_Var.Chainvar[position].getSize(); arc++)
		{
			Use_Test_Constraint_Chain[position][arc] = IloRange(TestChain_Var.Chainvar[position][arc] <= 0);
			Use_Test_Constraint_Chain[position][arc].setLinearCoef(Testvar[TestChain_Var.Link_Chainvar_Arc[position][arc]], -1);
		}
		model.add(Use_Test_Constraint_Chain[position]);
	}
	cout << "Second Bender Constraint Set" << endl;
	// Third set, all cycle variables must have one preceding and/or succeeding cycle arc. (No preceding for position 1, no succeeding for position config.cyclelength)
	vector<vector<IloRangeArray>> Constraint_Cycle_Succeeding(G.nr_pairs - 1); // (nr_pairs - 1) as the first index i the graph copy.
	for (int copy = 0; copy < G.nr_pairs - 1; copy++)
	{
		Constraint_Cycle_Succeeding[copy].resize(config.cyclelength-1);
		for (int position = 0; position < config.cyclelength-1; position++)
		{
			Constraint_Cycle_Succeeding[copy][position] = IloRangeArray(env, TestCycle_Var.Cyclevariable[copy][position].getSize());
			for (int arc = 0; arc < TestCycle_Var.Cyclevariable[copy][position].getSize(); arc++)
			{
				if (G.arcs[TestCycle_Var.Link_Cyclevar_Arc[copy][position][arc]].endvertex != copy) // If the endvertex is the copy vertex, there will be no succeeding arc.
				{
					Constraint_Cycle_Succeeding[copy][position][arc] = IloRange(TestCycle_Var.Cyclevariable[copy][position][arc] <= 0);
					int endvertex = G.arcs[TestCycle_Var.Link_Cyclevar_Arc[copy][position][arc]].endvertex;
					for (int arc2 = 0; arc2 < TestCycle_Var.Cyclevariable[copy][position + 1].getSize(); arc2++)
					{
						if (endvertex == G.arcs[TestCycle_Var.Link_Cyclevar_Arc[copy][position + 1][arc2]].startvertex)
							Constraint_Cycle_Succeeding[copy][position][arc].setLinearCoef(TestCycle_Var.Cyclevariable[copy][position + 1][arc2], -1);
					}
				}
			}
			model.add(Constraint_Cycle_Succeeding[copy][position]);
		}
	}
	vector<vector<IloRangeArray>> Constraint_Cycle_Preceding(G.nr_pairs - 1); // (nr_pairs - 1) as the first index i the graph copy.
	for (int copy = 0; copy < G.nr_pairs - 1; copy++)
	{
		Constraint_Cycle_Preceding[copy].resize(config.cyclelength);
		for (int position = 1; position < config.cyclelength; position++)
		{
			Constraint_Cycle_Preceding[copy][position] = IloRangeArray(env, TestCycle_Var.Cyclevariable[copy][position].getSize());
			for (int arc = 0; arc < TestCycle_Var.Cyclevariable[copy][position].getSize(); arc++)
			{
				Constraint_Cycle_Preceding[copy][position][arc] = IloRange(TestCycle_Var.Cyclevariable[copy][position][arc] <= 0);
				int startvertex = G.arcs[TestCycle_Var.Link_Cyclevar_Arc[copy][position][arc]].startvertex;
				for (int arc2 = 0; arc2 < TestCycle_Var.Cyclevariable[copy][position - 1].getSize(); arc2++)
				{
					if (startvertex == G.arcs[TestCycle_Var.Link_Cyclevar_Arc[copy][position - 1][arc2]].endvertex)
						Constraint_Cycle_Preceding[copy][position][arc].setLinearCoef(TestCycle_Var.Cyclevariable[copy][position - 1][arc2], -1);
				}
			}
			model.add(Constraint_Cycle_Preceding[copy][position]);
		}
	}
		cout << "Third Bender Constraint Set" << endl;
	// Final set does the same (require preceding variable) but for the chains
	vector<IloRangeArray> Constraint_Chain_Preceding(config.chainlength);
	for (int position = 0; position < config.chainlength-1; position++)
	{
		Constraint_Chain_Preceding[position] = IloRangeArray(env, TestChain_Var.Chainvar[position+1].getSize());
		for (int arc = 0; arc < TestChain_Var.Chainvar[position+1].getSize(); arc++)
		{
			Constraint_Chain_Preceding[position][arc] = IloRange(TestChain_Var.Chainvar[position+1][arc] <= 0);
			int startvertex = G.arcs[TestChain_Var.Link_Chainvar_Arc[position+1][arc]].startvertex;
			for (int arc2 = 0; arc2 < TestChain_Var.Chainvar[position].getSize(); arc2++)
			{
				if (startvertex = G.arcs[TestChain_Var.Link_Chainvar_Arc[position][arc2]].endvertex)
				{
					Constraint_Chain_Preceding[position][arc].setLinearCoef(TestChain_Var.Chainvar[position][arc2], -1);
				}	
			}
		}
	}
}
