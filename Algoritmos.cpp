#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <algorithm>
#include <iomanip>

using namespace std;

// Esta estructura guarda toda la información de un proceso
struct Process {
    string label;         // nombre del proceso
    int burstTime;        // tiempo total que necesita en CPU
    int arrivalTime;      // tiempo en el que llega
    int queueValue;       // cola a la que pertenece
    int priority;         // prioridad dentro de esa cola
    int remainingTime;    // tiempo que le falta por ejecutarse
    int startTime;        // primer instante en el que entra a CPU
    int finishTime;       // instante en el que termina
    int waitingTime;      // tiempo de espera
    int turnaroundTime;   // tiempo total desde que llega hasta que termina
    int responseTime;     // cuánto tardó en empezar por primera vez
    bool started;         // para saber si ya empezó al menos una vez
    bool finished;        // para saber si ya terminó
};

// Aquí se guarda qué algoritmo usa cada cola
struct QueuePolicy {
    string type;   // RR, SJF o STCF
    int quantum;   // solo sirve si la cola usa RR
};

// Quita espacios al inicio y al final de una cadena
string trim(string s) {
    int left = 0;
    int right = (int)s.size() - 1;
    string ans = "";
    while (left < (int)s.size() &&
          (s[left] == ' ' || s[left] == '\t' || s[left] == '\n' || s[left] == '\r')) {
        left++;
    }
    while (right >= 0 &&
          (s[right] == ' ' || s[right] == '\t' || s[right] == '\n' || s[right] == '\r')) {
        right--;
    }

    if (left <= right) {
        ans = s.substr(left, right - left + 1);
    }
    return ans;
}

// Reinicia todos los datos calculados de los procesos
void initializeProcesses(vector<Process>& processes) {
    for (int i = 0; i < (int)processes.size(); i++) {
        processes[i].remainingTime = processes[i].burstTime;
        processes[i].startTime = -1;
        processes[i].finishTime = -1;
        processes[i].waitingTime = 0;
        processes[i].turnaroundTime = 0;
        processes[i].responseTime = -1;
        processes[i].started = false;
        processes[i].finished = false;
    }
}

// Este comparador sirve para ordenar por llegada
// Si empatan, se desempata por cola, prioridad y label
bool compareArrival(Process a, Process b) {
    bool ans = false;
    if (a.arrivalTime < b.arrivalTime) {
        ans = true;
    } else if (a.arrivalTime == b.arrivalTime) {
        if (a.queueValue < b.queueValue) {
            ans = true;
        } else if (a.queueValue == b.queueValue) {
            if (a.priority > b.priority) {
                ans = true;
            } else if (a.priority == b.priority) {
                ans = a.label < b.label;
            }
        }
    }
    return ans;
}

// Carga los procesos desde el archivo de entrada
// Formato esperado: label;BT;AT;Q;Priority
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
                // ignora comentarios
                if (line[0] != '#') {
                    stringstream ss(line);
                    vector<string> parts;
                    string part;
                    while (getline(ss, part, ';')) {
                        parts.push_back(trim(part));
                    }
                    // si tiene los 5 campos, se arma el proceso
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
                        p.finished = false;
                        processes.push_back(p);
                    }
                }
            }
        }
        file.close();
    }
    return ok;
}

// Ya cuando el proceso terminó, aquí se calculan las métricas
void calculateMetrics(vector<Process>& processes) {
    for (int i = 0; i < (int)processes.size(); i++) {
        processes[i].turnaroundTime = processes[i].finishTime - processes[i].arrivalTime;
        processes[i].waitingTime = processes[i].turnaroundTime - processes[i].burstTime;
        processes[i].responseTime = processes[i].startTime - processes[i].arrivalTime;
    }
}

// Mira si en una cola de mayor prioridad ya hay procesos esperando
bool isHigherQueueWaiting(vector<queue<int> >& readyQueues, int queueIndex) {
    bool ans = false;
    int i = 0;
    while (i < queueIndex && !ans) {
        if (!readyQueues[i].empty()) {
            ans = true;
        }
        i++;
    }
    return ans;
}

// Mete a las colas todos los procesos que ya hayan llegado
void addArrivedProcesses(
    vector<Process>& processes,
    int currentTime,
    vector<bool>& added,
    vector<queue<int> >& readyQueues
) {
    vector<int> arrivals;
    // revisa cuáles todavía no se han agregado y ya llegaron
    for (int i = 0; i < (int)processes.size(); i++) {
        if (!added[i] && processes[i].arrivalTime <= currentTime) {
            arrivals.push_back(i);
            added[i] = true;
        }
    }
    // se ordenan para mantener un criterio fijo al entrar
    sort(arrivals.begin(), arrivals.end(), [&](int a, int b) {
        bool ans = false;
        if (processes[a].queueValue < processes[b].queueValue) {
            ans = true;
        } else if (processes[a].queueValue == processes[b].queueValue) {
            if (processes[a].priority > processes[b].priority) {
                ans = true;
            } else if (processes[a].priority == processes[b].priority) {
                if (processes[a].arrivalTime < processes[b].arrivalTime) {
                    ans = true;
                } else if (processes[a].arrivalTime == processes[b].arrivalTime) {
                    ans = processes[a].label < processes[b].label;
                }
            }
        }
        return ans;
    });
    // ahora sí se agregan a la cola que les toca
    for (int i = 0; i < (int)arrivals.size(); i++) {
        int index = arrivals[i];
        int q = processes[index].queueValue - 1;
        if (q >= 0 && q < 4) {
            readyQueues[q].push(index);
        }
    }
}

// Verifica si todos los procesos ya terminaron
bool allFinished(vector<Process>& processes) {
    bool ans = true;
    for (int i = 0; i < (int)processes.size(); i++) {
        if (!processes[i].finished) {
            ans = false;
        }
    }
    return ans;
}

// Escoge el mejor proceso de una cola para SJF
// Como SJF no es expropiativo aquí se elige uno y se deja fuera de la cola
int pickBestNonPreemptiveFromQueue(
    queue<int>& q,
    vector<Process>& processes,
    string policyType
) {
    vector<int> temp;
    int selected = -1;
    // saco todo de la cola para poder revisar quién es mejor
    while (!q.empty()) {
        temp.push_back(q.front());
        q.pop();
    }
    for (int i = 0; i < (int)temp.size(); i++) {
        int idx = temp[i];
        if (selected == -1) {
            selected = idx;
        } else {
            if (policyType == "SJF") {
                if (processes[idx].burstTime < processes[selected].burstTime) {
                    selected = idx;
                } else if (processes[idx].burstTime == processes[selected].burstTime) {
                    if (processes[idx].priority > processes[selected].priority) {
                        selected = idx;
                    } else if (processes[idx].priority == processes[selected].priority) {
                        if (processes[idx].arrivalTime < processes[selected].arrivalTime) {
                            selected = idx;
                        } else if (processes[idx].arrivalTime == processes[selected].arrivalTime) {
                            if (processes[idx].label < processes[selected].label) {
                                selected = idx;
                            }
                        }
                    }
                }
            }
        }
    }
    // devuelvo a la cola todos menos el escogido
    for (int i = 0; i < (int)temp.size(); i++) {
        if (temp[i] != selected) {
            q.push(temp[i]);
        }
    }
    return selected;
}

// Escoge el mejor proceso para STCF
// Aquí gana el que tenga menor tiempo restante
int pickBestPreemptiveFromQueue(
    queue<int>& q,
    vector<Process>& processes
) {
    vector<int> temp;
    int selected = -1;
    // saco todo de la cola temporalmente
    while (!q.empty()) {
        temp.push_back(q.front());
        q.pop();
    }
    for (int i = 0; i < (int)temp.size(); i++) {
        int idx = temp[i];
        if (selected == -1) {
            selected = idx;
        } else {
            if (processes[idx].remainingTime < processes[selected].remainingTime) {
                selected = idx;
            } else if (processes[idx].remainingTime == processes[selected].remainingTime) {
                if (processes[idx].priority > processes[selected].priority) {
                    selected = idx;
                } else if (processes[idx].priority == processes[selected].priority) {
                    if (processes[idx].arrivalTime < processes[selected].arrivalTime) {
                        selected = idx;
                    } else if (processes[idx].arrivalTime == processes[selected].arrivalTime) {
                        if (processes[idx].label < processes[selected].label) {
                            selected = idx;
                        }
                    }
                }
            }
        }
    }
    // regreso todos menos el que se va a ejecutar
    for (int i = 0; i < (int)temp.size(); i++) {
        if (temp[i] != selected) {
            q.push(temp[i]);
        }
    }
    return selected;
}

// Esta es la función principal del planificador MLFQ
void runMlfq(
    vector<Process>& processes,
    vector<QueuePolicy>& policies,
    vector<string>& timeline
) {
    vector<queue<int> > readyQueues(4);               // las 4 colas
    vector<bool> added((int)processes.size(), false); // para saber si ya entró al sistema
    int currentTime = 0;
    initializeProcesses(processes);
    // sigue mientras todavía exista al menos un proceso sin terminar
    while (!allFinished(processes)) {
        // primero se agregan los que ya llegaron en este tiempo
        addArrivedProcesses(processes, currentTime, added, readyQueues);
        // se busca la cola más prioritaria que tenga algo
        int activeQueue = -1;
        for (int i = 0; i < 4; i++) {
            if (!readyQueues[i].empty() && activeQueue == -1) {
                activeQueue = i;
            }
        }
        // si ninguna cola tiene procesos, la CPU queda idle
        if (activeQueue == -1) {
            timeline.push_back("t=" + to_string(currentTime) + " -> IDLE");
            currentTime++;
        } else {
            QueuePolicy policy = policies[activeQueue];
            // Si la cola usa Round Robin
            if (policy.type == "RR") {
                int index = readyQueues[activeQueue].front();
                readyQueues[activeQueue].pop();
                // si es la primera vez que entra a CPU, guardo el startTime
                if (!processes[index].started) {
                    processes[index].started = true;
                    processes[index].startTime = currentTime;
                }
                // corre máximo un quantum o lo que le falte
                int executionTime = policy.quantum;
                if (processes[index].remainingTime < executionTime) {
                    executionTime = processes[index].remainingTime;
                }
                int used = 0;
                bool stopByHigherQueue = false;
                while (used < executionTime && !stopByHigherQueue) {
                    timeline.push_back("t=" + to_string(currentTime) + " -> " + processes[index].label);
                    currentTime++;
                    processes[index].remainingTime--;
                    used++;
                    // después de cada unidad reviso si llegaron más procesos
                    addArrivedProcesses(processes, currentTime, added, readyQueues);
                    // si ya terminó, guardo su tiempo final
                    if (processes[index].remainingTime == 0) {
                        processes[index].finishTime = currentTime;
                        processes[index].finished = true;
                    } else {
                        // si apareció algo en una cola mejor, se detiene
                        if (isHigherQueueWaiting(readyQueues, activeQueue)) {
                            stopByHigherQueue = true;
                        }
                    }
                }
                // si todavía no termina, vuelve al final de su cola
                if (!processes[index].finished) {
                    readyQueues[activeQueue].push(index);
                }
            }
            //Si la cola usa SJF
            else if (policy.type == "SJF") {
                int index = pickBestNonPreemptiveFromQueue(readyQueues[activeQueue], processes, "SJF");
                if (!processes[index].started) {
                    processes[index].started = true;
                    processes[index].startTime = currentTime;
                }
                // como es no expropiativo, se ejecuta completo
                while (processes[index].remainingTime > 0) {
                    timeline.push_back("t=" + to_string(currentTime) + " -> " + processes[index].label);
                    currentTime++;
                    processes[index].remainingTime--;

                    addArrivedProcesses(processes, currentTime, added, readyQueues);
                }
                processes[index].finishTime = currentTime;
                processes[index].finished = true;
            }
            // Si la cola usa STCF
            else if (policy.type == "STCF") {
                int index = pickBestPreemptiveFromQueue(readyQueues[activeQueue], processes);
                if (!processes[index].started) {
                    processes[index].started = true;
                    processes[index].startTime = currentTime;
                }
                // STCF corre de a 1 unidad para poder reevaluar después
                timeline.push_back("t=" + to_string(currentTime) + " -> " + processes[index].label);
                currentTime++;
                processes[index].remainingTime--;
                addArrivedProcesses(processes, currentTime, added, readyQueues);
                if (processes[index].remainingTime == 0) {
                    processes[index].finishTime = currentTime;
                    processes[index].finished = true;
                } else {
                    // si no terminó, vuelve a competir con los otros de su cola
                    readyQueues[activeQueue].push(index);
                }
            }
        }
    }
    calculateMetrics(processes);
}

// Imprime la tabla de resultados en consola
void printResults(vector<Process> processes, string title) {
    double avgWaiting = 0.0;
    double avgFinish = 0.0;
    double avgResponse = 0.0;
    double avgTurnaround = 0.0;
    sort(processes.begin(), processes.end(), compareArrival);
    cout << "\n========================================\n";
    cout << title << "\n";
    cout << "========================================\n";
    cout << left
         << setw(10) << "Process"
         << setw(6)  << "BT"
         << setw(6)  << "AT"
         << setw(6)  << "Q"
         << setw(6)  << "Pr"
         << setw(8)  << "WT"
         << setw(8)  << "CT"
         << setw(8)  << "RT"
         << setw(8)  << "TAT" << "\n";
    for (int i = 0; i < (int)processes.size(); i++) {
        cout << left
             << setw(10) << processes[i].label
             << setw(6)  << processes[i].burstTime
             << setw(6)  << processes[i].arrivalTime
             << setw(6)  << processes[i].queueValue
             << setw(6)  << processes[i].priority
             << setw(8)  << processes[i].waitingTime
             << setw(8)  << processes[i].finishTime
             << setw(8)  << processes[i].responseTime
             << setw(8)  << processes[i].turnaroundTime << "\n";
        avgWaiting += processes[i].waitingTime;
        avgFinish += processes[i].finishTime;
        avgResponse += processes[i].responseTime;
        avgTurnaround += processes[i].turnaroundTime;
    }
    if ((int)processes.size() > 0) {
        avgWaiting /= (int)processes.size();
        avgFinish /= (int)processes.size();
        avgResponse /= (int)processes.size();
        avgTurnaround /= (int)processes.size();
    }
    cout << "\nWT=" << fixed << setprecision(2) << avgWaiting
         << "; CT=" << avgFinish
         << "; RT=" << avgResponse
         << "; TAT=" << avgTurnaround << ";\n";
}

// Guarda los resultados en un archivo de salida
void writeOutputFile(
    string outputFileName,
    vector<Process> processes,
    vector<string> timeline
) {
    ofstream out(outputFileName);
    double avgWaiting = 0.0;
    double avgFinish = 0.0;
    double avgResponse = 0.0;
    double avgTurnaround = 0.0;
    sort(processes.begin(), processes.end(), compareArrival);
    if (out.is_open()) {
        out << "# etiqueta; BT; AT; Q; Pr; WT; CT; RT; TAT\n";
        for (int i = 0; i < (int)processes.size(); i++) {
            out << processes[i].label << ";"
                << processes[i].burstTime << ";"
                << processes[i].arrivalTime << ";"
                << processes[i].queueValue << ";"
                << processes[i].priority << ";"
                << processes[i].waitingTime << ";"
                << processes[i].finishTime << ";"
                << processes[i].responseTime << ";"
                << processes[i].turnaroundTime << "\n";
            avgWaiting += processes[i].waitingTime;
            avgFinish += processes[i].finishTime;
            avgResponse += processes[i].responseTime;
            avgTurnaround += processes[i].turnaroundTime;
        }
        if ((int)processes.size() > 0) {
            avgWaiting /= (int)processes.size();
            avgFinish /= (int)processes.size();
            avgResponse /= (int)processes.size();
            avgTurnaround /= (int)processes.size();
        }
        out << fixed << setprecision(2);
        out << "WT=" << avgWaiting
            << "; CT=" << avgFinish
            << "; RT=" << avgResponse
            << "; TAT=" << avgTurnaround << ";\n\n";
        // también guarda la línea de tiempo por si la quieres revisar
        out << "# Timeline\n";
        for (int i = 0; i < (int)timeline.size(); i++) {
            out << timeline[i] << "\n";
        }
        out.close();
    }
}

// Arma la configuración de colas según el esquema elegido
vector<QueuePolicy> buildScheme(int schemeOption) {
    vector<QueuePolicy> policies(4);
    if (schemeOption == 1) {
        policies[0].type = "RR";   policies[0].quantum = 1;
        policies[1].type = "RR";   policies[1].quantum = 3;
        policies[2].type = "RR";   policies[2].quantum = 4;
        policies[3].type = "SJF";  policies[3].quantum = 0;
    } else if (schemeOption == 2) {
        policies[0].type = "RR";   policies[0].quantum = 2;
        policies[1].type = "RR";   policies[1].quantum = 3;
        policies[2].type = "RR";   policies[2].quantum = 4;
        policies[3].type = "STCF"; policies[3].quantum = 0;
    } else {
        policies[0].type = "RR";   policies[0].quantum = 3;
        policies[1].type = "RR";   policies[1].quantum = 5;
        policies[2].type = "RR";   policies[2].quantum = 6;
        policies[3].type = "RR";   policies[3].quantum = 20;
    }
    return policies;
}

// Solo devuelve un nombre bonito para mostrar el esquema
string schemeName(int schemeOption) {
    string ans = "";
    if (schemeOption == 1) {
        ans = "MLFQ -> RR(1), RR(3), RR(4), SJF";
    } else if (schemeOption == 2) {
        ans = "MLFQ -> RR(2), RR(3), RR(4), STCF";
    } else {
        ans = "MLFQ -> RR(3), RR(5), RR(6), RR(20)";
    }
    return ans;
}

int main(int argc, char* argv[]) {
    vector<Process> processes;
    vector<string> timeline;
    string inputFile = "mlq001.txt";
    string outputFile = "output_mlq001.txt";
    int schemeOption = 1;
    bool loaded = true;
    // si pasan argumentos por consola, se usan esos
    if (argc >= 2) {
        inputFile = argv[1];
    }
    if (argc >= 3) {
        schemeOption = stoi(argv[2]);
    }
    if (argc >= 4) {
        outputFile = argv[3];
    }
    loaded = loadProcesses(inputFile, processes);
    if (!loaded) {
        cout << "Could not open file: " << inputFile << "\n";
    } else {
        vector<QueuePolicy> policies = buildScheme(schemeOption);
        // corre toda la simulación
        runMlfq(processes, policies, timeline);
        // muestra resultados en consola
        printResults(processes, schemeName(schemeOption));
        // guarda resultados en archivo
        writeOutputFile(outputFile, processes, timeline);
        cout << "\nOutput file generated: " << outputFile << "\n";
    }
    return 0;
}