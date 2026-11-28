# Johns Hopkins University Ionosphere Database

## Sources

- Donor: Vince Sigillito (vgs@aplcen.apl.jhu.edu)
- Date: 1989
- Source: Space Physics Group, Applied Physics Laboratory, Johns Hopkins University, Johns Hopkins Road, Laurel, MD 20723

## Past Usage

Sigillito, V. G., Wing, S. P., Hutton, L. V., & Baker, K. B. (1989).
Classification of radar returns from the ionosphere using neural networks.
Johns Hopkins APL Technical Digest, 10, 262-266.

They investigated using backpropagation and the perceptron training algorithm
on this database. Using the first 200 instances for training, split almost
equally between positive and negative examples, they found that a linear
perceptron attained 90.7% accuracy, a non-linear perceptron attained 92%, and
backpropagation averaged over 96% accuracy on the remaining test instances.
The original description reports 150 test instances, consisting of 123 good
and 24 bad returns; these counts are inconsistent, and the complete data set
contains 351 instances rather than 350.

Accuracy on good instances was much higher than on bad instances.
Backpropagation was tested with different numbers of hidden units (0 to 15),
and incremental results were reported after different numbers of epochs.

David Aha (aha@ics.uci.edu) also investigated this database. He found that
nearest neighbour attained 92.1% accuracy, Ross Quinlan's C4 algorithm attained
94.0% (no windowing), and IB3 (Aha & Kibler, IJCAI-1989) attained 96.7%
(acceptance and dropping parameters of 70% and 80%, respectively).

## Relevant Information

This radar data was collected by a system in Goose Bay, Labrador. The system
consists of a phased array of 16 high-frequency antennas with a total
transmitted power of approximately 6.4 kilowatts. The targets were free
electrons in the ionosphere.

Good radar returns show evidence of some type of structure in the ionosphere.
Bad returns do not; their signals pass through the ionosphere.

Received signals were processed using an autocorrelation function whose
arguments are the time of a pulse and the pulse number. There were 17 pulse
numbers for the Goose Bay system. Each instance has two attributes per pulse
number, corresponding to the complex values returned by the function.

- Number of instances: 351
- Number of attributes: 34 continuous predictors plus the class
- Class distribution: 225 good returns and 126 bad returns
- Missing values: none

## Attribute Information

In `ionosphere.csv`, the first column is the class label:

- `g`: good radar return
- `b`: bad radar return

The remaining 34 columns contain the continuous predictor attributes in their
original order. The class column has been moved from the last position in the
original data set to the first position for ULTRA's default CSV format.
