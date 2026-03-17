#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iomanip>

using namespace std;

struct Process {
    string label;
    int burstTime;
    int arrivalTime;
    int queueValue;
    int priority;
    int remainingTime;
    int startTime;
    int finishTime;
    int waitingTime;
    int turnaroundTime;
    int responseTime;
    bool started;
};

string trim(string s) {
    int left = 0;
    int right = (int)s.size() - 1;
    string ans = "";
    while (left < (int)s.size() && (s[left] == ' ' || s[left] == '\t' || s[left] == '\n' || s[left] == '\r')) {
        left++;
    }
    while (right >= 0 && (s[right] == ' ' || s[right] == '\t' || s[right] == '\n' || s[right] == '\r')) {
        right--;
    }
    if (left <= right) {
        ans = s.substr(left, right - left + 1);
    }
    return ans;
}

void initializeProcesses(vector<Process>& processes) {
    for (int i = 0; i < (int)processes.size(); i++) {
        processes[i].remainingTime = processes[i].burstTime;
        processes[i].startTime = -1;
        processes[i].finishTime = -1;
        processes[i].waitingTime = 0;
        processes[i].turnaroundTime = 0;
        processes[i].responseTime = -1;
        processes[i].started = false;
    }
}

bool compareArrival(Process a, Process b) {
    bool ans = false;
    if (a.arrivalTime < b.arrivalTime) {
        ans = true;
    } else if (a.arrivalTime == b.arrivalTime) {
        ans = a.label < b.label;
    }
    return ans;
}

bool compareBurstArrival(Process a, Process b) {
    bool ans = false;
    if (a.arrivalTime < b.arrivalTime) {
        ans = true;
    } else if (a.arrivalTime == b.arrivalTime) {
        if (a.burstTime < b.burstTime) {
            ans = true;
        } else if (a.burstTime == b.burstTime) {
            ans = a.label < b.label;
        }
    }
    return ans;
}

bool loadProcesses(string fileName, vector<Process>& processes) {
    ifstream file(fileName);
    string line;
    bool ok = true;
    if (!file.is_open()) {
        ok = false;
    } else {
        while (getline(file, line)) {
            line = trim(line);
            if (line != "") {
                if (line[0] != '#') {
                    stringstream ss(line);
                    vector<string> parts;
                    string part;
                    while (getline(ss, part, ';')) {
                        parts.push_back(trim(part));
                    }
                    if ((int)parts.size() == 5) {
                        Process p;
                        p.label = parts[0];
                        p.burstTime = stoi(parts[1]);
                        p.arrivalTime = stoi(parts[2]);
                        p.queueValue = stoi(parts[3]);
                        p.priority = stoi(parts[4]);
                        p.remainingTime = p.burstTime;
                        p.startTime = -1;
                        p.finishTime = -1;
                        p.waitingTime = 0;
                        p.turnaroundTime = 0;
                        p.responseTime = -1;
                        p.started = false;
                        processes.push_back(p);
                    }
                }
            }
        }
        file.close();
    }
    return ok;
}

void calculateMetrics(vector<Process>& processes) {
    for (int i = 0; i < (int)processes.size(); i++) {
        processes[i].turnaroundTime = processes[i].finishTime - processes[i].arrivalTime;
        processes[i].waitingTime = processes[i].turnaroundTime - processes[i].burstTime;
        processes[i].responseTime = processes[i].startTime - processes[i].arrivalTime;
    }
}

void printResults(vector<Process> processes, string algorithmName) {
    double avgWaiting = 0.0;
    double avgTurnaround = 0.0;
    double avgResponse = 0.0;
    sort(processes.begin(), processes.end(), compareArrival);
    cout << "\n========================================\n";
    cout << "Algorithm: " << algorithmName << "\n";
    cout << "========================================\n";
    cout << left
         << setw(10) << "Process"
         << setw(8)  << "BT"
         << setw(8)  << "AT"
         << setw(8)  << "Start"
         << setw(8)  << "Finish"
         << setw(10) << "Waiting"
         << setw(12) << "Turnaround"
         << setw(10) << "Response" << "\n";
    for (int i = 0; i < (int)processes.size(); i++) {
        cout << left
             << setw(10) << processes[i].label
             << setw(8)  << processes[i].burstTime
             << setw(8)  << processes[i].arrivalTime
             << setw(8)  << processes[i].startTime
             << setw(8)  << processes[i].finishTime
             << setw(10) << processes[i].waitingTime
             << setw(12) << processes[i].turnaroundTime
             << setw(10) << processes[i].responseTime << "\n";

        avgWaiting += processes[i].waitingTime;
        avgTurnaround += processes[i].turnaroundTime;
        avgResponse += processes[i].responseTime;
    }
    if ((int)processes.size() > 0) {
        avgWaiting = avgWaiting / processes.size();
        avgTurnaround = avgTurnaround / processes.size();
        avgResponse = avgResponse / processes.size();
    }
    cout << "\nAverage waiting time: " << fixed << setprecision(2) << avgWaiting << "\n";
    cout << "Average turnaround time: " << fixed << setprecision(2) << avgTurnaround << "\n";
    cout << "Average response time: " << fixed << setprecision(2) << avgResponse << "\n";
}

void runSjf(vector<Process>& processes) {
    int completed = 0;
    int currentTime = 0;
    int n = (int)processes.size();
    initializeProcesses(processes);
    while (completed < n) {
        int selected = -1;
        for (int i = 0; i < n; i++) {
            if (processes[i].remainingTime > 0 && processes[i].arrivalTime <= currentTime) {
                if (selected == -1) {
                    selected = i;
                } else {
                    if (processes[i].burstTime < processes[selected].burstTime) {
                        selected = i;
                    } else if (processes[i].burstTime == processes[selected].burstTime) {
                        if (processes[i].arrivalTime < processes[selected].arrivalTime) {
                            selected = i;
                        } else if (processes[i].arrivalTime == processes[selected].arrivalTime) {
                            if (processes[i].label < processes[selected].label) {
                                selected = i;
                            }
                        }
                    }
                }
            }
        }
        if (selected == -1) {
            currentTime++;
        } else {
            if (!processes[selected].started) {
                processes[selected].started = true;
                processes[selected].startTime = currentTime;
            }
            currentTime += processes[selected].burstTime;
            processes[selected].remainingTime = 0;
            processes[selected].finishTime = currentTime;
            completed++;
        }
    }
    calculateMetrics(processes);
}

int main(int argc, char* argv[]) {
    vector<Process> originalProcesses;
    string fileName = "inputE01.txt";
    bool loaded = true;
    if (argc >= 2) {
        fileName = argv[1];
    }
    loaded = loadProcesses(fileName, originalProcesses);
    if (!loaded) {
        cout << "Could not open file: " << fileName << "\n";
    } else {
        vector<Process> sjfProcesses = originalProcesses;
        runSjf(sjfProcesses);
        printResults(sjfProcesses, "SJF");
    }
    return 0;
}