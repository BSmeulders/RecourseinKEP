#include "stdafx.h"
#include "Structures.h"
#include "Functions.h"

using namespace std;

void Deterministic_KEP(configuration & config, directedgraph G)
{
	matching_result results = Cycle_Formulation_Linear(G, config);
	matching_result results2 = Cycle_Formulation_Integer(G, config);
	cout << "LP = " << results.objective << endl;
	cout << "Int = " << results2.objective << endl;
	cin.get();
}

matching_result Cycle_Formulation_Linear(directedgraph G, configuration & config)
{
	// This model uses position-indexed arc variables for the chains, and variables for each possible cycle.
	IloEnv env;
	IloModel Cycle_Linear(env);

	// Create the Variables. (pre-processing can be implemented here)
	vector<cycle_arcs> cycles = Find_Cycles(G, config);
	IloNumVarArray cyclevar(env, cycles.size() , 0, 1, ILOFLOAT);

	// Create the Objective Function
	IloObjective obj = IloMaximize(env);
	for (int i = 0; i < cycles.size(); i++)
	{
		obj.setLinearCoef(cyclevar[i], cycles[i].weight);
	}
	Cycle_Linear.add(obj);


	// Build the constraints.
	IloRangeArray vertex_cons(env, G.size, 0, 1);
	for (int i = 0; i < cycles.size(); i++)
	{
		for (int j = 0; j < cycles[i].vertices.size(); j++)
		{
			vertex_cons[cycles[i].vertices[j]].setLinearCoef(cyclevar[i], 1);
		}
	}
	Cycle_Linear.add(vertex_cons);

	IloCplex CPLEX(Cycle_Linear);
	//CPLEX.exportModel("Cycle_Linear.lp");
	CPLEX.solve();

	matching_result results;
	results.objective = CPLEX.getObjValue();

	return results;
}
matching_result Cycle_Formulation_Integer(directedgraph G, configuration & config)
{
	// This model uses position-indexed arc variables for the chains, and variables for each possible cycle.
	IloEnv env;
	IloModel Cycle_Linear(env);

	// Create the Variables. (pre-processing can be implemented here)
	vector<cycle_arcs> cycles = Find_Cycles(G, config);
	IloNumVarArray cyclevar(env, cycles.size(), 0, 1, ILOINT);

	// Create the Objective Function
	// Currently an unweigted placeholder.
	IloObjective obj = IloMaximize(env);
	for (int i = 0; i < cycles.size(); i++)
	{
		obj.setLinearCoef(cyclevar[i], cycles[i].weight);
	}
	Cycle_Linear.add(obj);


	// Build the model.
	IloRangeArray vertex_cons(env, G.size, 0, 1);
	for (int i = 0; i < cycles.size(); i++)
	{
		for (int j = 0; j < cycles[i].vertices.size(); j++)
		{
			vertex_cons[cycles[i].vertices[j]].setLinearCoef(cyclevar[i], 1);
		}
	}
	Cycle_Linear.add(vertex_cons);

	IloCplex CPLEX(Cycle_Linear);
	//CPLEX.exportModel("Cycle_Linear.lp");
	CPLEX.solve();

	matching_result results;
	results.objective = CPLEX.getObjValue();

	return results;
}