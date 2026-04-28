#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <string>

using namespace std;


// Structure representing a fuzzy term with triangular membership function
struct FuzzyTerm {
    string name;      // Term name (e.g., "Close", "Far")
    double left;      // Left boundary of triangle
    double peak;      // Peak of triangle (membership = 1.0)
    double right;     // Right boundary of triangle

    FuzzyTerm(string n, double l, double p, double r)
        : name(n), left(l), peak(p), right(r) {}
};

// Structure representing a fuzzy inference rule: IF (input1 OP input2) THEN output
struct Rule {
    int inputTerm1Index;   // Index of first input term
    int inputTerm2Index;   // Index of second input term
    string logicOperator;  // Logical operator: "AND" (min) or "OR" (max)
    int outputTermIndex;   // Index of output term
    double weight;         // Rule weight (default: 1.0)

    Rule(int i1, int i2, string op, int out, double w = 1.0)
        : inputTerm1Index(i1), inputTerm2Index(i2), logicOperator(op),
        outputTermIndex(out), weight(w) {}
};

// Structure holding system variables: inputs and output
struct Variables {
    double distance;    // Input: distance to target (meters)
    double wind;        // Input: wind speed (m/s)
    double correction;  // Output: sight correction (angular minutes)
};


// Calculates membership degree for a triangular fuzzy set
// Returns value in range [0.0, 1.0]
// Parameters: value - crisp input value, term - fuzzy term definition
double calculateMembership(double value, const FuzzyTerm& term) {
    if (value <= term.left || value >= term.right) {
        return 0.0;
    }
    if (value >= term.left && value <= term.peak) {
        return (value - term.left) / (term.peak - term.left);
    }
    if (value >= term.peak && value <= term.right) {
        return (term.right - value) / (term.right - term.peak);
    }
    return 0.0;
}

// Fuzzification stage: converts crisp inputs to membership degrees
// Populates membership vectors for distance and wind terms
void fuzzify(const Variables& inputs,
    const vector<FuzzyTerm>& distanceTerms,
    const vector<FuzzyTerm>& windTerms,
    vector<double>& membershipDistance,
    vector<double>& membershipWind) {

    membershipDistance.clear();
    for (const auto& term : distanceTerms) {
        membershipDistance.push_back(calculateMembership(inputs.distance, term));
    }

    membershipWind.clear();
    for (const auto& term : windTerms) {
        membershipWind.push_back(calculateMembership(inputs.wind, term));
    }
}

// Inference stage: activates rules and aggregates conclusions
// Returns vector of activation levels for output terms
vector<double> infer(const vector<Rule>& rules,
    const vector<double>& membershipDistance,
    const vector<double>& membershipWind,
    int outputTermsCount) {

    vector<double> activatedValues(outputTermsCount, 0.0);

    for (const auto& rule : rules) {
        double activationLevel = 0.0;

        if (rule.logicOperator == "AND") {
            activationLevel = min(membershipDistance[rule.inputTerm1Index],
                membershipWind[rule.inputTerm2Index]);
        }
        else if (rule.logicOperator == "OR") {
            activationLevel = max(membershipDistance[rule.inputTerm1Index],
                membershipWind[rule.inputTerm2Index]);
        }

        activationLevel *= rule.weight;

        if (activationLevel > activatedValues[rule.outputTermIndex]) {
            activatedValues[rule.outputTermIndex] = activationLevel;
        }
    }

    return activatedValues;
}

// Defuzzification stage: converts fuzzy result to crisp value using centroid method
// Formula: result = Σ(peak_i × activation_i) / Σ(activation_i)
double defuzzify(const vector<double>& activatedValues,
    const vector<FuzzyTerm>& outputTerms) {

    double numerator = 0.0;
    double denominator = 0.0;

    for (size_t i = 0; i < outputTerms.size(); i++) {
        double representativeValue = outputTerms[i].peak;
        double activationLevel = activatedValues[i];

        numerator += representativeValue * activationLevel;
        denominator += activationLevel;
    }

    if (denominator == 0.0) {
        return 0.0;
    }

    return numerator / denominator;
}

// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main() {
    setlocale(LC_ALL, "Russian");
    cout << fixed << setprecision(2);

    cout << " " << endl;
    cout << "   FUZZY INFERENCE SYSTEM FOR BALLISTIC CALCULATIONS" << endl;
    cout << " " << endl;
    cout << endl;

    // Initialize input variable: Distance to target (meters)
    vector<FuzzyTerm> distanceTerms = {
        FuzzyTerm("Close",    0,   0,   300),
        FuzzyTerm("Medium",   200, 500,  800),
        FuzzyTerm("Far",      700, 1000, 1300)
    };

    // Initialize input variable: Wind speed (m/s)
    vector<FuzzyTerm> windTerms = {
        FuzzyTerm("Weak",      0, 0, 5),
        FuzzyTerm("Moderate",  3, 6, 9),
        FuzzyTerm("Strong",    7, 12, 17)
    };

    // Initialize output variable: Sight correction (angular minutes)
    vector<FuzzyTerm> correctionTerms = {
        FuzzyTerm("Minimal",   0, 0, 2),
        FuzzyTerm("Medium",    1, 3, 5),
        FuzzyTerm("Maximum",   4, 7, 10)
    };

    // Initialize rule base
    vector<Rule> rules = {
        Rule(0, 0, "AND", 0, 1.0),  // Close + Weak -> Minimal
        Rule(0, 1, "AND", 0, 1.0),  // Close + Moderate -> Minimal
        Rule(1, 0, "AND", 1, 1.0),  // Medium + Weak -> Medium
        Rule(1, 1, "AND", 1, 1.0),  // Medium + Moderate -> Medium
        Rule(2, 2, "AND", 2, 1.0),  // Far + Strong -> Maximum
        Rule(2, 2, "OR", 2, 1.0),   // Far OR Strong -> Maximum
        Rule(0, 2, "AND", 1, 1.0),  // Close + Strong -> Medium
        Rule(2, 0, "AND", 1, 1.0)   // Far + Weak -> Medium
    };

    cout << "Term base initialized:" << endl;
    cout << "  Distance: " << distanceTerms.size() << " terms" << endl;
    cout << "  Wind: " << windTerms.size() << " terms" << endl;
    cout << "  Correction: " << correctionTerms.size() << " terms" << endl;
    cout << "  Rules: " << rules.size() << " rules" << endl;
    cout << endl;

    char continueCalculation = 'y';

    while (continueCalculation == 'y' || continueCalculation == 'Y') {
        Variables inputs;

        cout << "----------------------------------------------------------------" << endl;
        cout << "Enter distance to target (meters, 0-1300): ";
        cin >> inputs.distance;

        cout << "Enter wind speed (m/s, 0-17): ";
        cin >> inputs.wind;

        if (inputs.distance < 0 || inputs.distance > 1300 ||
            inputs.wind < 0 || inputs.wind > 17) {
            cout << "Error: values out of valid range!" << endl;
            continue;
        }

        cout << endl;
        cout << "Processing data..." << endl;
        cout << endl;

        // Stage 1: Fuzzification
        vector<double> membershipDistance;
        vector<double> membershipWind;

        fuzzify(inputs, distanceTerms, windTerms, membershipDistance, membershipWind);

        cout << "Stage 1: Fuzzification" << endl;
        cout << "  Membership degrees for distance:" << endl;
        for (size_t i = 0; i < distanceTerms.size(); i++) {
            cout << "    " << distanceTerms[i].name << ": "
                << membershipDistance[i] << endl;
        }

        cout << "  Membership degrees for wind:" << endl;
        for (size_t i = 0; i < windTerms.size(); i++) {
            cout << "    " << windTerms[i].name << ": "
                << membershipWind[i] << endl;
        }
        cout << endl;

        // Stage 2: Inference
        vector<double> activatedValues = infer(rules, membershipDistance,
            membershipWind,
            correctionTerms.size());

        cout << "Stage 2: Inference (rule activation)" << endl;
        cout << "  Activated output terms:" << endl;
        for (size_t i = 0; i < correctionTerms.size(); i++) {
            cout << "    " << correctionTerms[i].name << ": "
                << activatedValues[i] << endl;
        }
        cout << endl;

        // Stage 3: Defuzzification
        inputs.correction = defuzzify(activatedValues, correctionTerms);

        cout << "Stage 3: Defuzzification (centroid method)" << endl;
        cout << "  Final sight correction: +" << inputs.correction
            << " angular minutes" << endl;
        cout << endl;

        // Output result
        cout << "================================================================" << endl;
        cout << "CALCULATION RESULT" << endl;
        cout << "================================================================" << endl;
        cout << "Distance: " << inputs.distance << " m" << endl;
        cout << "Wind: " << inputs.wind << " m/s" << endl;
        cout << "SIGHT CORRECTION: +" << inputs.correction << " ang. min" << endl;
        cout << "================================================================" << endl;
        cout << endl;

        cout << "Perform another calculation? (y/n): ";
        cin >> continueCalculation;
        cout << endl;
    }

    cout << "================================================================" << endl;
    cout << "Program finished successfully." << endl;
    cout << "================================================================" << endl;

    return 0;
}