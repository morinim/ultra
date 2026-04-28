#!/usr/bin/env python3

import os
import argparse


verbose = False


def calculate(dice, outfile):
    with open(outfile,"w") as out:
        sup = 6 ** dice
        for num in range(sup):
            line = []
            val = 0
            for j in reversed(range(dice)):
                die = 1 + (num // (6 ** j)) % 6
                if die%2 > 0:
                    val += die-1
                if j == dice-1:
                    line.append(str(die))
                else:
                    line.append("," + str(die))
            line = [str(val) + ","] + line
            out.write("".join(line) + "\n")


def get_cmd_line_options():
    description = "Generates the Petalrose dataset"
    parser = argparse.ArgumentParser(description = description)

    parser.add_argument("-d","--dice", type=int, default=5,
                        help="Generate petalrose file for n dice")
    parser.add_argument("-v","--verbose", action="store_true",
                        help="Turn on verbose mode")
    parser.add_argument("dataset")
    return parser


def main():
    # Get argument flags and command options
    parser = get_cmd_line_options()
    args = parser.parse_args()

    verbose = args.verbose

    calculate(args.dice, args.dataset)


if __name__ == "__main__":
    main()
