#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0
#
# The core script for ANCK kconfig checking.
# It is not recommended to call directly.
#
# Copyright (C) 2024 Qiao Ma <mqaio@linux.alibaba.com>

import argparse, re
from typing import List, Type, Dict, Tuple
from enum import Enum

def die(args: str):
    print(args)
    exit(1)

def default_args_func(args):
    pass

class Config():
    name: str
    value: str

    def __init__(self, name, value) -> None:
        self.name = name
        self.value = value

    @staticmethod
    def from_text(line: str) -> Type["Config"] :
        RE_CONFIG_SET = r'^(CONFIG_\w+)=(.*)$'
        RE_CONFIG_NOT_SET = r'^# (CONFIG_\w+) is not set$'

        if re.match(RE_CONFIG_SET, line):
            obj = re.match(RE_CONFIG_SET, line)
            return Config(name=obj.group(1), value=obj.group(2))
        elif re.match(RE_CONFIG_NOT_SET, line):
            obj = re.match(RE_CONFIG_NOT_SET, line)
            return Config(name=obj.group(1), value="n")
        return None

class ConfigList():
    configs: Dict[str, Config]

    def __init__(self) -> None:
        self.configs = {}

    @staticmethod
    def from_file(file: str) -> Type["ConfigList"]:
        confs = ConfigList()
        with open(file) as f:
            for line in f.readlines():
                conf = Config.from_text(line)
                if conf is None:
                    continue
                confs.configs[conf.name] = conf
        return confs

    def get(self, name) -> Type["Config"]:
        return self.configs.get(name, None)

ResultKind = Enum("ResultKind", ("SUCCESS", "MISS", "WRONG_VALUE", "NOT_IN_CHOICE", "NOT_IN_RANGE", "EXCLUSIVE_ERROR"))
RuleLevel = Enum("RuleLevel", ("L0_MANDATORY", "L1_RECOMMEND"))

class CheckResult():
    name: str
    kind: ResultKind
    level: RuleLevel
    value: str

    def __init__(self, level: RuleLevel, kind: ResultKind, name: str, text: str) -> None:
        self.level = level
        self.kind = kind
        self.name = name
        self.text = text

    def is_fatal_error(self):
        return self.kind != ResultKind.SUCCESS and self.level == RuleLevel.L0_MANDATORY

    def __str__(self) -> str:
        if self.kind == ResultKind.SUCCESS:
            return ""
        if self.level == RuleLevel.L0_MANDATORY:
            return f"ERROR: {self.text}\n"
        return f"WARNING: {self.text}\n"

    @staticmethod
    def success():
        return CheckResult(RuleLevel.L0_MANDATORY, ResultKind.SUCCESS, "", "")

    @staticmethod
    def miss(level: RuleLevel, name: str):
        return CheckResult(level, ResultKind.MISS, name, f"missed: {name}")

    @staticmethod
    def group_miss(level: RuleLevel, confs: List[str]):
        conf_list = " ".join(confs)
        return CheckResult(level, ResultKind.MISS, "", f"missed: none of follow configs exist {conf_list}")

    @staticmethod
    def wrong_value(level: RuleLevel, name: str, expected: str, real: str):
        return CheckResult(level, ResultKind.WRONG_VALUE, name,
                           f"wrong_value: {name}, expected: {expected}, real: {real}")

    @staticmethod
    def not_in_choice(level: RuleLevel, name: str, real_value: str, values: List[str]):
        str_values = ",".join(values)
        return CheckResult(level, ResultKind.NOT_IN_CHOICE, name,
                           f"not_in_choice: {name} {real_value} not in [{str_values}]")

    @staticmethod
    def not_in_range(level: RuleLevel, name: str, real_value: int, start: int, end: int):
        return CheckResult(level, ResultKind.NOT_IN_RANGE, name, f"not_in_range: {name} {real_value} not in range [{start}, {end}]")

    @staticmethod
    def exlusive_error(level: RuleLevel, confs: List[str]):
        str_confs = ",".join(confs)
        return CheckResult(level, ResultKind.EXCLUSIVE_ERROR, "", f"exclusive error: expected only one appears, but follow configs appears: {str_confs}")

class Rule():
    subclasses = []

    def __init_subclass__(cls, **kwargs):
        super().__init_subclass__(**kwargs)
        Rule.subclasses.append(cls)

    @staticmethod
    def try_parse(line: str, level: RuleLevel):
        raise NotImplementedError

    def check(self, line: str, level: RuleLevel):
        raise NotImplementedError

    @staticmethod
    def parse(line: str, level: RuleLevel):
        for subclass in Rule.subclasses:
            result = subclass.try_parse(line, level)
            if result is not None:
                return result
        die(f"cannot parse : {line}")

class RuleList():
    rules: List[Rule]

    def __init__(self):
        self.rules = []

    @staticmethod
    def from_file(path: str, level: RuleLevel) -> Type["RuleList"]:
        rl = RuleList()
        with open(path) as f:
            for line in f.readlines():
                line = line.strip()
                if line == "" or line.startswith("##"):
                    continue
                rule = Rule.parse(line, level)
                rl.rules.append(rule)
        return rl

    def check(self, confs: ConfigList) -> List[CheckResult]:
        results : List[CheckResult] = []
        for rule in self.rules:
            res = rule.check(confs)
            results.append(res)
        return results

    def merge(self, rhs: ConfigList):
        self.rules.extend(rhs.rules)

class ValueRule(Rule):
    conf: Config
    level: RuleLevel

    @staticmethod
    def try_parse(line: str, level: RuleLevel):
        rule = ValueRule()
        conf = Config.from_text(line)
        if conf is None:
            return None
        rule.conf = conf
        rule.level = level
        return rule

    def check(self, confs: ConfigList):
        name = self.conf.name
        conf = confs.get(name)
        if conf is None:
            return CheckResult.miss(self.level, self.conf.name)
        if conf.value != self.conf.value:
            return CheckResult.wrong_value(self.level, name, self.conf.value, conf.value)
        return CheckResult.success()

class UnlimitedRule(Rule):
    @staticmethod
    def try_parse(line: str, level: RuleLevel):
        RE_CONF_UNLIMITED = r'^# UNLIMITED CONFIG_\w+$'
        if not re.match(RE_CONF_UNLIMITED, line):
            return None
        return UnlimitedRule()

    def check(self, confs: ConfigList):
        return CheckResult.success()

class ChoiceRule(Rule):
    name: str
    values: List[str]

    def __init__(self, level, name, values) -> None:
        self.level = level
        self.name = name
        self.values = values

    @staticmethod
    def try_parse(line: str, level: RuleLevel):
        RE_CONF_CHOICE = r'^#\s*CHOICE\s+(CONFIG_\w+)\s+([\w,\/]+)$'
        obj = re.match(RE_CONF_CHOICE, line)
        if obj is None:
            return None
        name = obj.group(1)
        values = obj.group(2)
        return ChoiceRule(level, name, values.split("/"))

    def check(self, confs: ConfigList):
        conf = confs.get(self.name)
        if conf is None:
            return CheckResult.miss(self.level, self.name)
        if conf.value not in self.values:
            return CheckResult.not_in_choice(self.level, self.name, conf.value, self.values)
        return CheckResult.success()

class RangeRule(Rule):
    level: RuleLevel
    name: str
    start: int
    end: int

    def __init__(self, level: RuleLevel, name: str, start: int, end: int) -> None:
        self.level = level
        self.name = name
        self.start = start
        self.end = end

    @staticmethod
    def try_parse(line: str, level: RuleLevel):
        RE_CONF_RANGE = r'^#\s*RANGE\s+(CONFIG_\w+)\s+(\d+)\,(\d+)$'
        obj = re.match(RE_CONF_RANGE, line)
        if obj is None:
            return None
        return RangeRule(level, obj.group(1), int(obj.group(2)), int(obj.group(3)))

    def check(self, confs: ConfigList):
        conf = confs.get(self.name)
        if conf is None:
            return CheckResult.miss(self.level, self.name)
        val = int(conf.value)
        if val <= self.end and val >= self.start:
            return CheckResult.success()
        return CheckResult.not_in_range(self.level, self.name, val, self.start, self.end)

class ExclusiveRule(Rule):
    level: RuleLevel
    value: str
    confs: List[str]

    def __init__(self, level: RuleLevel, value: str, confs: List[str]) -> None:
        self.level = level
        self.value = value
        self.confs = confs

    @staticmethod
    def try_parse(line: str, level: RuleLevel):
        """# EXCLUSIVE value CONFIG_XXX [CONFIG_XXX ...]"""
        RE_CONF_RANGE = r'^#\s*EXCLUSIVE\s+(\w+)\s+(.*)$'
        obj = re.match(RE_CONF_RANGE, line)
        if obj is None:
            return None
        value = obj.group(1)
        confs = obj.group(2).split()
        if len(confs) == 0:
            return None
        return ExclusiveRule(level, value, confs)

    def check(self, confs: ConfigList):
        appears : List[Config] = []
        for name in self.confs:
            conf = confs.get(name)
            if conf is not None and conf.value != 'n':
                appears.append(conf)
        if len(appears) == 0:
            return CheckResult.group_miss(self.level, appears)
        if len(appears) != 1:
            return CheckResult.exlusive_error(self.level, [x.name for x in appears])
        if appears[0].value != self.value:
            return CheckResult.wrong_value(self.level, appears[0].name, self.value, appears[0].value)
        return CheckResult.success()

def level_of(l: str) -> RuleLevel:
    if l == "L0-MANDATORY":
        return RuleLevel.L0_MANDATORY
    elif l == "L1-RECOMMEND":
        return RuleLevel.L1_RECOMMEND
    die(f"unknown level {l}")

def do_check(args):
    confs = ConfigList.from_file(args.config)
    rules = RuleList()

    if len(args.rules) != len(args.level):
        die("the num of level and rules do not match")

    for i, rule_file in enumerate(args.rules):
        rules.merge(RuleList.from_file(rule_file, level_of(args.level[i])))
    results = rules.check(confs)

    fatal_error = False
    result_text = ""
    for r in results:
        result_text += str(r)
        fatal_error = fatal_error or r.is_fatal_error()

    if result_text == "":
        result_text = "PASS\n"
    print(result_text)
    exit(fatal_error)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='check configs')
    parser.set_defaults(func=default_args_func)
    subparsers = parser.add_subparsers()

    checker = subparsers.add_parser("check")
    checker.add_argument("--rules", action='append', default=[], help="the kconfig checking rule files")
    checker.add_argument("--level", action='append', default=[], help="the kconfig checking rule files")
    checker.add_argument("config", help="the config files to be checked")
    checker.set_defaults(func=do_check)

    args = parser.parse_args()
    args.func(args)
