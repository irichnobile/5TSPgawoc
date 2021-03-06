/*******************************************************************************
//  5TSPgawoc.cpp                   Author: Ian Nobile
//  Section: 50                     Due Date: 1 November 2021
//
//  This program solves a Travelling Salesperson Problem dataset with up to 222 
//  cities using a Wisdom of Crowds algorithm that aggregates the best solutions 
//  generated by a genetic algorithm (SCX, by Zakir Ahmed) with random mutations 
//  (RSM) and uses them to build consensus on a good-enough path and distance. 
//  It reports a set of relevant statistics at the end of program execution
//
*******************************************************************************/

constexpr int N = ???; // change to the number of cities in your .TSP file

#include <iostream> // print to console
#include <chrono>   // time the speed of the program
#include <fstream>  // read files
#include <string>   // header buffer for seeking inside a file
#include <vector>   // easy arraying
#include <cmath>    // distance formula
#include <algorithm>// easy array shuffling
#include <random>   // easy array shuffling
#include <list>     // constr. final tour
#include <C:\Users\admin\Desktop\coastal_islander\me\uofl\ai\project5\5TSPgawoc\matplotlib-cpp-master\matplotlibcpp.h>

using namespace std;
using namespace std::chrono;
namespace plt = matplotlibcpp;

// class declarations:
class Node {
public:
    int num;
    float x;
    float y;
};

class Tour {
public:
    array<int, N> nArray;
    float dist;
    array<bool, N + 1> legitimate;
};

// function prototypes:
vector<Node> buildGraph(char*);
float distCheck(Node, Node);
float calcTourDist(const array<array<float, N + 1>, N + 1>&, array<int, N>);  //??
Tour scx(const array<array<float, N + 1>, N + 1>&, Tour, Tour, bool);



//------------------------------------------------------------------------------
//  Main Function
//------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    // check if path was passed as arg:
    if (argc == 1) {
        cout << "Please pass the path to the .TSP file as a command line argument" << endl;
        return 1;
    }

    auto start = high_resolution_clock::now();  // start timer

    // begin with a friendly greeting
    cout << "Hello and welcome to the (Genetic Algorithm - Wisdom of Crowds) Travelling Salesperson Problem Solver" << endl << endl;

    // fill graph
    vector<Node> graph = buildGraph(argv[1]);

    /*
    // graphing:
    vector<double> xPlot;
    vector<double> yPlot;
    plt::xkcd();
    for (Node node : graph) {
        xPlot.push_back(node.x);
        yPlot.push_back(node.y);
        plt::plot(xPlot, yPlot,"-o");
        xPlot.pop_back();
        yPlot.pop_back();
    }
    plt::show();
    */

    // calculate all possible distances up front (saves cycles))
    array<array<float, N + 1>, N + 1> distRef;
    for (int i = 0; i < (N + 1); i++) {
        for (int j = 0; j < (N + 1); j++) {
            if (i == 0 || j == 0) {
                distRef[i][j] = 0.0;    // highly improves readability of dist checks
            } else {
                distRef[i][j] = distCheck(graph[(i - 1)], graph[(j - 1)]);
            }
        }
    }

    //WoC Cycle:
    array<Tour, 100> crowd;
    for (int crowdCounter = 0; crowdCounter < crowd.size(); crowdCounter++) {

        // create an initial population of N totally randomised tours
        array<Tour, N> pop;
        for (int i = 0;i < pop.size();i++) {
            Tour tour = Tour();
            fill(tour.legitimate.begin(), tour.legitimate.end(), true); // bool array initialisation
            tour.legitimate[0] = false; // no destination 0

            // shuffle nodes of graph object
            unsigned seed = system_clock::now().time_since_epoch().count();
            shuffle(graph.begin(), graph.end(), std::default_random_engine(seed));

            for (int j = 0;j < N;j++) {
                tour.nArray[j] = graph[j].num;
            }

            //set tour distance and store in pop
            tour.dist = calcTourDist(distRef, tour.nArray);
            pop[i] = tour;
        }
        sort(graph.begin(), graph.end(), [](Node a, Node b) { return a.num < b.num; }); // re-sort graph

        // prep mutation probability
        srand(system_clock::now().time_since_epoch().count());
        default_random_engine generator;
        std::bernoulli_distribution distribution(0.1); // for 10% of offspring
        bool mutate = false;

        //Main Loop:
        for (int upperBound = 1;upperBound < 101;upperBound++) {
            // sort by dist
            sort(pop.begin(), pop.end(), [](Tour a, Tour b) { return a.dist < b.dist; });

            // randomly mate top N/2 fittest and store offspring in newPop
            array<Tour, N> newPop;
            for (int i = 0; i < N; i++) {
                srand(system_clock::now().time_since_epoch().count());
                int p1Index = rand() % (N/2), p2Index = rand() % (N/2);
                mutate = false;
                if (distribution(generator)) { mutate = true; }
                newPop[i] = scx(distRef, pop[p1Index], pop[p2Index], mutate);
            }
            pop = newPop;
        }   //end Main Loop

        sort(pop.begin(), pop.end(), [](Tour a, Tour b) { return a.dist < b.dist; });

        // put #1 at front
        while (*pop[0].nArray.begin() != 1) {
            rotate(pop[0].nArray.begin(), pop[0].nArray.begin() + 1, pop[0].nArray.end());
        }
        
        crowd[crowdCounter] = pop[0];
    }// End WoC Cycle

    sort(crowd.begin(), crowd.end(), [](Tour a, Tour b) { return a.dist < b.dist; });
    
    cout << "This diverse crowd has " << crowd.size() << " members. The min is " << crowd[0].dist << " and the max is " << crowd[crowd.size() - 1].dist << endl;
    // calc average
    float avg = 0;
    for (int i = 0;i < crowd.size();i++) {
        avg += crowd[i].dist;
    }
    avg /= crowd.size();
    cout << "The crowd currently believe that the optimum tour should be around " << avg << " in length" << endl << endl;

    // find the most common distance:
    float current = crowd[0].dist, previous = 0.0;
    int count = 1, max = 1, maxB = 0;
    for (int i = 1; i < crowd.size(); i++) {
        previous = current;
        current = crowd[i].dist;
        if (current == previous) {
            count++;
        } else {
            if (count > max) {
                maxB = i - 1;
                max = count;
            }
            count = 1;
        }
    }
    if (count > max) {
        maxB = crowd.size() - 1;
        max = count;
    }
    int maxA = maxB - max + 1;  // create bounds of mcpd tours

    // crowdsource most common path:
    array<bool, crowd.size()> legitimate;
    Tour crowdTour = Tour();
    fill(legitimate.begin(), legitimate.end(), false);
    for (int i = maxA; i < maxB + 1; i++) {
        legitimate[i] = true;
    }
    list<int> nextStops;
    current = 0, previous = 0, count = 0;
    int iMax = 0, next = 0, iTour = maxA;

    for (int j = 1; j < N; j++) {
        for (int i = maxA;i < maxB + 1;i++) {
            if (legitimate[i] == true) {
                rotate(crowd[i].nArray.begin(), crowd[i].nArray.begin() + 1, crowd[i].nArray.end());
                nextStops.push_back(crowd[i].nArray[0]);
                iTour = i;
            }
        }
        if (nextStops.size() == 1) {
            break;
        }
        nextStops.sort();

        current = nextStops.front(), count = 1, iMax = 1, next = current;
        nextStops.pop_front();
        while (nextStops.size() > 0) {
            previous = current;
            current = nextStops.front();
            nextStops.pop_front();
            if (current == previous) {
                count++;
            } else {
                if (count > iMax) {
                    next = previous;
                    iMax = count;
                }
                count = 1;
            }
        }
        if (count > iMax) {
            next = previous;
            iMax = count;
        }
        for (int i = maxA; i < maxB + 1; i++) {
            if (crowd[i].nArray[0] != next) {
                legitimate[i] = false;
            }
        }
    }
    crowdTour.nArray = crowd[iTour].nArray;
    while (crowdTour.nArray[0] != 1) {
        rotate(crowdTour.nArray.begin(), crowdTour.nArray.begin() + 1, crowdTour.nArray.end());
    }
    crowdTour.dist = calcTourDist(distRef, crowdTour.nArray);

    // report res
    cout << "The crowd believe the optimum tour to be: " << endl << endl << "\t";
    for (int i = 0; i < N;i++) {
        cout << crowdTour.nArray[i];
        //xPlot.push_back(graph[pop[0].nArray[i] - 1].x);
        //yPlot.push_back(graph[pop[0].nArray[i] - 1].y);
        //plt::plot(xPlot, yPlot);
        //plt::pause(1);
        cout << " -> ";
    }
    cout << crowdTour.nArray[0] << endl;
    //xPlot.push_back(graph[pop[0].nArray[0] - 1].x);
    //yPlot.push_back(graph[pop[0].nArray[0] - 1].y);
    //plt::plot(xPlot, yPlot);
    //plt::pause(1);
    //plt::show();

    cout << endl << "The actual distance of this tour is " << crowdTour.dist << ", " << avg - crowdTour.dist << " less than that originally projected" << endl;
    if ((int)crowdTour.dist == (int)crowd[0].dist) {
        cout << "but equal to the current optimal path, " << crowd[0].dist << "!" << endl << endl;
    } else if (crowdTour.dist > crowd[0].dist){
        cout << "but also just " << crowdTour.dist - crowd[0].dist << " more than the current optimal path, " << crowd[0].dist << "!" << endl << endl;
    } else {
        cout << "but also just " << crowd[0].dist - crowdTour.dist << " less than the current optimal path, " << crowd[0].dist << "!" << endl << endl;
    }
    
    count = 0;
    for (int i = 0; i < legitimate.size(); i++) {
        if (legitimate[i] == true) {
            count++;
        }
    }
    cout << (float)max / crowd.size() << "% of the crowd believed this would be the actual distance, " << endl;
    cout << "and "<< (float)count / legitimate.size() << "% had both this distance and the final path predicted in combination" << endl << endl;

    auto stop = high_resolution_clock::now();   // stop timer
    auto duration = duration_cast<microseconds>(stop - start);  // calculate elapsed time
    cout << "Program execution took " << duration.count() / 1000000.0 << "s" << endl << endl;

    system("pause");
    return 0;
}



//------------------------------------------------------------------------------
//  Function Definitions:
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  Read .TSP file, creates nodes from coordinates and combines all in an
//  undirected graph object
//------------------------------------------------------------------------------
vector<Node> buildGraph(char* argv) {
    // open .TSP file in read-only mode:
    ifstream tspfile;
    tspfile.open(argv, ios::in);
    // ensure file exists:
    if (!tspfile.is_open()) {
        vector<Node> graph;
        return graph;
    }
    vector<Node> graph;
    int dimension = 0;
    string heading = "";
    // advance buffer to the dimension section:
    while (heading.compare("DIMENSION:") != 0) {
        tspfile >> heading;
    }
    tspfile >> dimension;
    // advance buffer to the coordinates section:
    while (heading.compare("NODE_COORD_SECTION") != 0) {
        tspfile >> heading;
    }
    // create nodes and push to graph vector
    Node newNode = Node();
    for (int i = 0;i < dimension;i++) {
        tspfile >> newNode.num;
        tspfile >> newNode.x;
        tspfile >> newNode.y;
        graph.push_back(newNode);
    }
    // The graph is now created, and we are finished with the .TSP file
    tspfile.close();
    return graph;
}

//------------------------------------------------------------------------------
//  Returns the distance between two graph nodes using the pythagoran formula: 
//  dist = sqrt((x2 - x1)^2 + (y2 - y1)^2)
//------------------------------------------------------------------------------
float distCheck(Node first, Node second) {
    float dx = second.x - first.x;
    float dy = second.y - first.y;
    return sqrt(dx * dx + dy * dy);
}

//------------------------------------------------------------------------------
//  Calculates the length of a tour
//------------------------------------------------------------------------------
float calcTourDist(const array<array<float, N + 1>, N + 1> &distRef, array<int, N> tour) {
    float minDist = 0.0;
    for (int i = 0;i < N - 1;i++) {
        minDist += distRef[tour[i]][tour[i + 1]];
    }
    minDist += distRef[tour[N - 1]][tour[0]];
    return minDist;
}

//------------------------------------------------------------------------------
//  An implementation of SCX, by Zakir H. Ahmed
//------------------------------------------------------------------------------
Tour scx(const array<array<float, N + 1>, N + 1>& distRef, Tour p1, Tour p2, bool mutate) {
    Tour o = Tour();
    fill(o.legitimate.begin(), o.legitimate.end(), true);
    o.legitimate[0] = false;    // no destination 0 in tsp data
    o.nArray[0] = p1.nArray[0]; // start with first dest of first parent
    o.legitimate[o.nArray[0]] = false;
    int index1 = 0, index2 = 0;
    float dist1 = 0.0, dist2 = 0.0;

    for (int i = 0; i < N - 1; i++) {
        // position runners
        index1 = 0, index2 = 0;
        while (p1.nArray[index1] != o.nArray[i]) { index1++; }
        index1++;
        index1 %= N;
        while (!o.legitimate[p1.nArray[index1]]) { index1++; index1 %= N; }
        while (p2.nArray[index2] != o.nArray[i]) { index2++; }
        index2++;
        index2 %= N;
        while (!o.legitimate[p2.nArray[index2]]) { index2++; index2 %= N; }

        // calculate the distances
        dist1 = distRef[o.nArray[i]][p1.nArray[index1]];
        dist2 = distRef[o.nArray[i]][p2.nArray[index2]];

        // append the closer node
        o.nArray[i + 1] = dist1 < dist2 ? p1.nArray[index1] : p2.nArray[index2];
        o.legitimate[o.nArray[i + 1]] = false;
    }

    // relegitimise o
    fill(o.legitimate.begin(), o.legitimate.end(), true);
    o.legitimate[0] = false;

    // mutation
    if (mutate) {
        index1 = rand() % N;
        index2 = rand() % N;

        // mutation 2 (rsm)
        if (index1 > index2) { swap(index1, index2); }
        while (index1 < index2) {
            swap(o.nArray[index1++], o.nArray[index2--]);
        }
    };

    o.dist = calcTourDist(distRef, o.nArray);
    return o;
}

