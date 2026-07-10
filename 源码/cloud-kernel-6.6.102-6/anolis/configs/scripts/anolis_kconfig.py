#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0
#
# The core script for ANCK kconfig baseline
# It is not recommended to call directly.
#
# Copyright (C) 2023 Qiao Ma <mqaio@linux.alibaba.com>

import argparse, re, os, glob, shutil, copy
from typing import List, Dict, Type, Callable, Tuple
import json
from collections import Counter
import fnmatch
import functools

def die(*args, **kwargs):
    print(*args, **kwargs)
    exit(1)

class Rules():
    @staticmethod
    def levels() -> List[str]:
        return ["L0-MANDATORY", "L1-RECOMMEND", "L2-OPTIONAL", "UNKNOWN"]

    @staticmethod
    def base_dist():
        return "ANCK"

    @staticmethod
    def as_config_text(name: str, value: str) -> str:
        if value is None or value == "n":
            return f"# {name} is not set\n"
        else:
            return f"{name}={value}\n"

class PathIterContext():
    dist: str
    level: str
    arch: str
    subarch: str
    name: str
    path: str
    data: any

    def __init__(self, data: any, dist: str, level: str, arch: str, subarch: str, name: str, path: str) -> None:
        self.data = data
        self.dist = dist
        self.level = level
        self.arch = arch
        self.subarch = subarch
        self.name = name
        self.path = path

class PathManager():
    @staticmethod
    def dists(top_dir: str, dists: List[str] = None) -> List[str]:
        dist_list = [Rules.base_dist()]
        override_dir = os.path.join(top_dir, "OVERRIDE")
        if os.path.exists(override_dir):
            dist_list.extend(os.listdir(override_dir))

        if dists is None:
            return dist_list
        return list(set(dist_list).intersection(set(dists)))

    @staticmethod
    def dist_to_path(top_dir: str, dist: str) -> str:
        if dist == Rules.base_dist():
            return top_dir
        return os.path.join(top_dir, "OVERRIDE", dist)

    @staticmethod
    def levels(dist_dir: str, levels: List[str] = None) -> List[str]:
        all_levels = []
        for d in os.listdir(dist_dir):
            if not os.path.isdir(os.path.join(dist_dir, d)):
                continue
            if not re.match('^L[0-9].*|UNKNOWN', d):
                continue
            all_levels.append(d)

        if levels is None:
            return all_levels
        return list(set(all_levels).intersection(set(levels)))

    @staticmethod
    def archs(variant_dir: str, archs: List[str] = None) -> List[str]:
        all_archs = os.listdir(variant_dir)
        return all_archs if archs is None else list(set(all_archs).intersection(set(archs)))

    @staticmethod
    def __for_each_arch(level_dir: str, data: any, func: Callable[[PathIterContext], None], dist: str, level: str, archs: List[str] = None, subarchs: List[str] = None):
        for arch_dir in os.listdir(level_dir):
            if "-" in arch_dir:
                arch, subarch = arch_dir.split("-", maxsplit=1)
            else:
                arch = arch_dir
                subarch = None
            if archs is not None and arch not in archs:
                continue
            if subarchs is not None and subarch is not None and subarch not in subarchs:
                continue
            full_arch_dir = os.path.join(level_dir, arch_dir)
            for conf in os.listdir(full_arch_dir):
                path = os.path.join(full_arch_dir, conf)
                context = PathIterContext(data, dist, level, arch, subarch, conf, path)
                func(context)

    @staticmethod
    def for_each(top_dir: str, data: any, func: Callable[[PathIterContext], None], dists: List[str] = None, levels: List[str] = None, archs: List[str] = None, subarchs: List[str] = None):
        for dist in PathManager.dists(top_dir, dists):
            dist_dir = PathManager.dist_to_path(top_dir, dist)
            for level in PathManager.levels(dist_dir, levels):
                level_dir = os.path.join(dist_dir, level)
                if level != "UNKNOWN":
                    PathManager.__for_each_arch(level_dir, data, func, dist, level, archs, subarchs)
                else:
                    for conf in os.listdir(level_dir):
                        PathManager.__for_each_arch(os.path.join(level_dir, conf), data, func, dist, level, archs, subarchs)

    @staticmethod
    def as_level_dir(top_dir: str, dist: str, level: str):
        path = PathManager.dist_to_path(top_dir, dist)
        path = os.path.join(path, level)
        return path

    @staticmethod
    def as_path(top_dir: str, dist: str, level: str, arch: str, subarch: str, name: str):
        path = PathManager.as_level_dir(top_dir, dist, level)
        if level == "UNKNOWN":
            path = os.path.join(path, name)
        if subarch is None:
            path = os.path.join(path, arch, name)
        else:
            path = os.path.join(path, f"{arch}-{subarch}", name)
        return path


def default_args_func(args):
    pass

class LevelInfo():
    info: Dict[str, str]

    def __init__(self) -> None:
        self.info = {}

    def get(self, conf: str) -> str:
        return self.info.get(conf, "UNKNOWN")

    def merge_with_base(self, base: Type["LevelInfo"]):
        if base is None:
            return
        for name, level in base.info.items():
            if name not in self.info:
                self.info[name] = level

    @staticmethod
    def __collect_info(ctx: PathIterContext):
        level_info: Dict[str, str] = ctx.data
        level_info[ctx.name] = ctx.level

    @staticmethod
    def build(path: str, dist: str) -> Type["LevelInfo"]:
        info = LevelInfo()
        PathManager.for_each(path, info.info, LevelInfo.__collect_info, dists=[dist])
        return info

    @staticmethod
    def load(file: str):
        info = LevelInfo()
        with open(file) as f:
            info.info = json.loads(f.read())
        return info

class Config():
    name: str
    value: str
    arch: str
    subarch: str
    level: str
    dist: str

    def __init__(self, name: str, value: str, dist: str = None, level: str = "UNKNOWN", arch: str = None, subarch: str = None) -> None:
        self.name = name
        self.value = value
        self.dist = dist
        self.level = level
        self.arch = arch
        self.subarch = subarch

    @staticmethod
    def from_text(line: str, dist: str, arch: str, subarch: str) -> Type["Config"] :
        RE_CONFIG_SET = r'^(CONFIG_\w+)=(.*)$'
        RE_CONFIG_NOT_SET = r'^# (CONFIG_\w+) is not set$'

        if re.match(RE_CONFIG_SET, line):
            obj = re.match(RE_CONFIG_SET, line)
            return Config(name=obj.group(1), value=obj.group(2), dist=dist, arch=arch, subarch=subarch)
        elif re.match(RE_CONFIG_NOT_SET, line):
            obj = re.match(RE_CONFIG_NOT_SET, line)
            return Config(name=obj.group(1), value="n", dist=dist, arch=arch, subarch=subarch)
        return None

    def as_text(self) -> str:
        return Rules.as_config_text(self.name, self.value)

    def as_path(self, top_dir: str) -> str:
        return PathManager.as_path(top_dir, self.dist, self.level, self.arch, self.subarch, self.name)

    def as_file(self, top_dir: str):
        text = self.as_text()
        path = self.as_path(top_dir)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w") as f:
            f.write(text)

class ConfigList():
    arch: str
    dist: str
    subarch: str
    configs: Dict[str, Config]

    def __init__(self, dist: str, arch: str, subarch: str = None) -> None:
        self.dist = dist
        self.arch = arch
        self.subarch = subarch
        self.configs = {}

    def lists(self) -> List[Config]:
        return list(self.configs.values())

    def diff_to_base(self, base: Type["ConfigList"], level_info: LevelInfo):
        same_configs = []
        for name, conf in self.configs.items():
            if name not in base.configs:
                continue
            if conf.value != base.configs[name].value:
                continue
            same_configs.append(name)

        for name in base.configs:
            if name not in self.configs:
                self.configs[name] = Config(name, value=None, dist=self.dist, arch=self.arch, level=level_info.get(name))

        for name in same_configs:
            del self.configs[name]

    def merge_with_base(self, base: Type["ConfigList"]):
        if base is None:
            return
        for name, conf in base.configs.items():
            if name not in self.configs:
                self.configs[name] = conf

    def dump_as_file(self, top_dir: str):
        for conf in self.configs.values():
            conf.as_file(top_dir)

    def as_text(self):
        text = ""
        for conf in self.configs.values():
            text = text + conf.as_text()
        return text

    @staticmethod
    def from_path(path: str, dist: str, arch: str, subarch: str = None, level_info: LevelInfo = None, level: str = None) -> Type["ConfigList"]:
        if level_info is not None and level is not None:
            die("the argument level_info and level cannot be passed together")
        if level_info is None and level is None:
            level = "UNKNOWN"

        conflist = ConfigList(dist, arch, subarch)
        with open(path) as f:
            for line in f.readlines():
                conf = Config.from_text(line, dist, arch, subarch)
                if conf is None:
                    continue
                if level_info is not None:
                    conf.level = level_info.get(conf.name)
                else:
                    conf.level = level
                conflist.configs[conf.name] = conf
        return conflist

class LevelCollector():
    @staticmethod
    def do_collect(args):
        info = LevelInfo.build(args.top_dir, args.dist)
        if args.base is not None:
            base_info = None
            for base in args.base:
                cur_base = LevelInfo.build(args.top_dir, base)
                cur_base.merge_with_base(base_info)
                base_info = cur_base
            info.merge_with_base(base_info)
        print(json.dumps(info.info, ensure_ascii=False, indent=2))

class Importer():
    @staticmethod
    def do_import(args):
        level_info = LevelInfo.load(args.level_info)
        conflist = ConfigList.from_path(path=args.config, dist=args.dist, arch=args.arch, subarch=args.subarch, level_info=level_info)
        conflist.dump_as_file(args.top_dir)

class Generator():
    @staticmethod
    def collect_config(ctx: PathIterContext):
        conflist : ConfigList = ctx.data
        cur_conf = ConfigList.from_path(path=ctx.path, dist=ctx.dist, arch=ctx.arch, subarch=ctx.subarch)
        conflist.merge_with_base(cur_conf)

    @staticmethod
    def do_generate(args):
        dist = args.dist
        archdir = args.archdir
        if "-" in archdir:
            arch, subarch = archdir.split("-", maxsplit=1)
        else:
            arch, subarch = archdir, None
        conflist = ConfigList(dist, arch, subarch)
        subarchs = None if subarch is None else [subarch]
        PathManager.for_each(args.top_dir, conflist, Generator.collect_config, dists=[dist], archs=[arch], subarchs=subarchs)
        print(conflist.as_text())

class Merger():
    @staticmethod
    def do_merge(args):
        conflist = None
        for file in args.file:
            cur_conflist = ConfigList.from_path(file, dist="", arch="")
            cur_conflist.merge_with_base(conflist)
            conflist = cur_conflist

        print(conflist.as_text())

class Collapser():
    # for configs, the keys are: conf_name, arch
    configs: Dict[str, Dict[str, Config]]
    archs: set

    def __init__(self) -> None:
        self.configs = {}
        self.archs = set()

    @staticmethod
    def __do_collect_info(ctx: PathIterContext):
        c: Collapser = ctx.data
        configs: Dict[str, Dict[str, Config]] = c.configs
        archs = c.archs

        full_arch = ctx.arch
        if ctx.subarch is not None:
            full_arch = f"{ctx.arch}-{ctx.subarch}"
        archs.add(full_arch)

        conflist = ConfigList.from_path(path=ctx.path, dist=ctx.dist, arch=ctx.arch, subarch=ctx.subarch, level=ctx.level)
        for conf in conflist.lists():
            if conf.name not in configs:
                configs[conf.name] = {}
            configs[conf.name][full_arch] = conf

    @staticmethod
    def __collapse_one_config(arch_confs: Dict[str, Config], archs: set, top_dir: str):
        # the default value is only depends on arch x86 and arm64.
        # For example:
        # 1. the configs "x86 y, arm64 y, sw_64 m/n" will be collpased to "default y, sw_64 m/n"
        # 2. the configs "x86 y, arm64 y, sw_64 y" will be collpased to "default y"
        # 3. the configs "x86 y, arm64 m, sw_64 y" will not be collpased
        if "x86" not in arch_confs or "arm64" not in arch_confs:
            return
        if arch_confs["x86"].value != arch_confs["arm64"].value:
            return
        common_conf = copy.deepcopy(arch_confs["x86"])
        common_conf.arch = "default"
        common_conf.subarch = None

        for arch in archs:
            if arch in arch_confs:
                conf = arch_confs[arch]
                if conf.value == common_conf.value:
                    os.remove(conf.as_path(top_dir))
            else:
                miss_conf = copy.deepcopy(common_conf)
                miss_conf.arch = arch
                miss_conf.subarch = None
                miss_conf.value = "n"
                miss_conf.as_file(top_dir)
        common_conf.as_file(top_dir)

    @staticmethod
    def do_collapse(args):
        c = Collapser()
        PathManager.for_each(args.top_dir, c, Collapser.__do_collect_info, dists=[args.dist])

        for arch_confs in c.configs.values():
            Collapser.__collapse_one_config(arch_confs, c.archs, args.top_dir)

class Striper():
    configs: Dict[str, List[str]]
    file_list: List[str]

    def __init__(self, file_list: List[str]) -> None:
        self.configs = {}
        self.file_list = file_list

        for i, path in enumerate(file_list):
            conflist = ConfigList.from_path(path, dist="", arch="")
            for conf in conflist.lists():
                name = conf.name
                if name not in self.configs:
                    self.configs[name] = [None]*i
                self.configs[name].append(conf.value)
            for conf_values in self.configs.values():
                if len(conf_values) != i+1:
                    conf_values.append(None)

    def strip(self, base: Type["Striper"]):
        disappear_confs = []
        same_confs = []
        for name, conf_values in base.configs.items():
            if name not in self.configs:
                disappear_confs.append(name)
                continue
            if conf_values == self.configs[name]:
                same_confs.append(name)

        for name in same_confs:
            del self.configs[name]

        num_files = len(self.file_list)
        for name in disappear_confs:
            self.configs[name] = [None]*num_files

    def override_files(self):
        for i, path in enumerate(self.file_list):
            with open(path, "w") as f:
                for name, values in self.configs.items():
                    f.write(Rules.as_config_text(name, values[i]))

    @staticmethod
    def do_strip(args):
        if len(args.base) != len(args.target):
            die("the target config files do not match base")
        base = Striper(args.base)
        target = Striper(args.target)
        target.strip(base)
        target.override_files()

class ImportOpTranslater():
    files: Dict[str, str]
    files_info: Dict[Tuple[str, str, str, str], str]
    level_info_path: str
    input_dir: str
    output_dir: str
    src_root: str

    file_counter = 0

    def __init__(self, input_dir: str, output_dir: str, src_root: str) -> None:
        self.files = {}
        self.files_info = {}
        self.input_dir = input_dir
        self.output_dir = output_dir
        self.src_root = src_root
        self.level_info_path = ""

    def __cmd(self, cmd: str):
        return f"python3 {__file__} {cmd} "

    def __op_file(self, args: str):
        # FILE dist arch variant file_path REFRESH/NOREFRESH

        # use file_counter to make file name unique
        ImportOpTranslater.file_counter += 1
        dist, arch, subarch, path, refresh = args.split()
        new_path = os.path.join(self.output_dir, f"{ImportOpTranslater.file_counter}-{os.path.basename(path)}")
        if subarch != "null":
            self.files[f"{dist}-{arch}-{subarch}"] = new_path
            self.files_info[(dist, arch, subarch)] = new_path
        else:
            self.files[f"{dist}-{arch}"] = new_path
            self.files_info[(dist, arch, None)] = new_path
        cmd = f"cp {path} {new_path}\n"
        if refresh == "REFRESH":
            cmd += f"make KCONFIG_CONFIG={new_path} ARCH={arch} CROSS_COMPILE=scripts/dummy-tools/ "
            cmd += f"PAHOLE=scripts/dummy-tools/pahole "
            cmd += f"-C {self.src_root} olddefconfig > /dev/null 2>&1\n"
            cmd += f"rm -f {new_path}.old \n"
        return cmd

    def __op_levelinfo(self, args: str):
        #LEVELINFO target_dist [base_dist [base_dist ...]]
        target_dist, base_dists = args.split(maxsplit=1)
        cmd = self.__cmd("collect_level")
        cmd += f"--dist {target_dist} --top_dir {self.input_dir} "
        for base in base_dists.split():
            if base == "null":
                continue
            cmd += f"--base {base} "
        self.level_info_path = os.path.join(self.output_dir, "level_info")
        cmd += f"> {self.level_info_path}"
        return cmd

    def __op_import(self, args: str):
        # IMPORT file
        file = args
        subarch = None
        dist, arch = file.split("-", maxsplit=1)
        if "-" in arch:
            arch, subarch = arch.split("-", maxsplit=1)

        cmd = self.__cmd("import")
        cmd += f"--dist {dist} --arch {arch} "
        if subarch is not None:
            cmd += f"--subarch {subarch} "
        cmd += f"--level_info {self.level_info_path} --top_dir {self.output_dir} "
        cmd += f"{self.files[file]} "
        return cmd

    def __op_collapse(self, args: str):
        # COLLAPSE dist
        dist = args
        cmd = self.__cmd("collapse")
        cmd += f"--dist {dist} --top_dir {self.output_dir}"
        return cmd

    def __op_strip(self, args: str):
        # STRIP target_dist base_dist
        target_dist, base_dist = args.split()
        copy_cmd = ""
        cmd = self.__cmd("strip")
        for (dist, arch, subarch), target_path in self.files_info.items():
            if dist != target_dist:
                continue
            try:
                copy_cmd += f"cp {target_path} {target_path}.bak\n"
                base_path = self.files_info[(base_dist, arch, subarch)]
            except:
                full_arch = arch
                if subarch is not None:
                    full_arch = f"{arch}-{subarch}"
                die(f"strip error. cannot find file {base_dist}-{full_arch} to match {target_dist}-{full_arch}")
            cmd += f"--base {base_path} --target {target_path} "
        return copy_cmd + cmd

    def __translate_one(self, op:str, args: str):
        cmd = ""
        if op == "FILE":
            cmd = self.__op_file(args)
        elif op == "LEVELINFO":
            cmd = self.__op_levelinfo(args)
        elif op == "IMPORT":
            cmd = self.__op_import(args)
        elif op == "COLLAPSE":
            cmd = self.__op_collapse(args)
        elif op == "STRIP":
            cmd = self.__op_strip(args)
        else:
            die(f"unknown op {op}")
        print(cmd)

    @staticmethod
    def do_translate(args):
        t = ImportOpTranslater(input_dir=args.input_dir, output_dir=args.output_dir, src_root=args.src_root)
        with open(args.path) as f:
            for i, line in enumerate(f.readlines()):
                line = line.strip()
                if line.startswith("#") or line == "":
                    continue
                (op, action_args) = line.split(maxsplit=1)
                try:
                    t.__translate_one(op, action_args)
                except:
                    die(f"parse error in {args.path}:{i+1}\n> {line}")

class KconfigLayoutEntry():
    name: str
    dist: str
    arch: str
    subarch: str
    base_dist: str
    base_name: str
    # (dist, variant, arch)
    layout_list: List[Tuple[str, str, str]]

    def __init__(self, name: str, dist: str, arch: str, base_dist: str, base_name: str) -> None:
        self.name = name
        self.dist = dist
        self.arch = arch
        self.base_dist = base_dist
        self.base_name = base_name
        self.layout_list = []

    @staticmethod
    def from_text(line: str):
        cur, arch, base, layouts = line.split()
        dist, name = cur.split("/")
        if base == "null":
            base_dist = None
            base_name = None
        else:
            base_dist, base_name = base.split("/")
        entry = KconfigLayoutEntry(name, dist, arch,base_dist, base_name)
        for l in layouts.split(";"):
            variant, arch = l.split("/")
            entry.layout_list.append((dist, variant, arch))
        return entry

class KconfigLayout():
    # (dist, file_name)
    layouts: Dict[Tuple[str, str], KconfigLayoutEntry]

    def __init__(self) -> None:
        self.layouts = {}

    @staticmethod
    def from_path(path: str) -> Type["KconfigLayout"]:
        l = KconfigLayout()
        with open(path) as f:
            for line in f.readlines():
                line = line.strip()
                if line.startswith("#") or line == "":
                    continue
                e = KconfigLayoutEntry.from_text(line)
                l.layouts[(e.dist, e.name)] = e

                if e.base_dist is None:
                    continue
                if (e.base_dist, e.base_name) not in l.layouts:
                    die(f"cannot find {e.base_dist}/{e.base_name} while parsing {e.dist}/{e.name}")
                e.layout_list = l.layouts[(e.base_dist, e.base_name)].layout_list + e.layout_list
        return l

class GenerateTranslater():
    input_dir: str
    output_dir: str
    src_root: str

    def __init__(self, args) -> None:
        self.input_dir = args.input_dir
        self.output_dir = args.output_dir
        self.src_root = args.src_root

    def __cmd(self, cmd: str):
        return f"python3 {__file__} {cmd} "

    def __translate_one(self, e: KconfigLayoutEntry, tmp_dir: str):
        files = []
        cmd = ""
        for dist, variant, arch in e.layout_list:
            if variant == "generic":
                # for geneic configs, generate them
                file = os.path.join(tmp_dir, f"kernel-partial-{dist}-{variant}-{arch}.config")
                cmd += self.__cmd("generate")
                cmd += f"--top_dir {self.input_dir} --dist {dist} --archdir {arch}"
                cmd += f"> {file} \n"
                files.append(file)
            else:
                dist_path = PathManager.dist_to_path(self.input_dir, dist)
                file = os.path.join(dist_path, "custom-overrides", variant, f"{arch}.config")
                if os.path.exists(file):
                    files.append(file)

        # merge all partial configs
        final_path = os.path.join(self.output_dir, f"kernel-{e.dist}-{e.name}.config")
        cmd += self.__cmd("merge")
        cmd += " ".join(files)
        cmd += f" > {final_path} \n"

        # refresh configs
        # cmd += f"echo \"* generated file: {final_path}\"\n"
        cmd += f"make KCONFIG_CONFIG={final_path} ARCH={e.arch} CROSS_COMPILE=scripts/dummy-tools/ "
        cmd += f"PAHOLE=scripts/dummy-tools/pahole "
        cmd += f"-C {self.src_root} olddefconfig > /dev/null 2>&1\n"
        cmd += f"rm -f {final_path}.old \n"
        cmd += f"echo \"* processed file: {final_path}\"\n"

        return cmd

    @staticmethod
    def do_translate(args):
        cmd = ""
        t = GenerateTranslater(args)
        l = KconfigLayout.from_path(args.layout)

        tmp_dir = os.path.join(args.output_dir, "tmp")
        cmd += f"mkdir -p {tmp_dir}\n"
        if args.target is not None:
            dist, file_name = args.target.split("/", maxsplit=1)
            if (dist, file_name) not in l.layouts:
                die(f"cannot find config layout info for {dist}/{file_name}")
            cmd += t.__translate_one(l.layouts[((dist, file_name))], tmp_dir)
        else:
            for e in l.layouts.values():
                cmd += t.__translate_one(e, tmp_dir)
        cmd += f"rm -rf {tmp_dir}"
        print(cmd)

class Mover():
    """move configs from old level to new level"""
    config_patterns: List[str]
    new_level: str
    top_dir: str

    def __init__(self, top_dir: str, new_level: str, config_patterns: List[str]) -> None:
        self.top_dir = top_dir
        self.new_level = new_level
        self.config_patterns = config_patterns

    @staticmethod
    def get_level(level: str) -> str:
        target_level = ""
        for l in Rules.levels():
            if l.startswith(level):
                if target_level != "":
                    die(f"the level {level} is ambiguous")
                target_level = l

        if target_level == "":
            die(f"unkonw level {level}")
        return target_level

    @staticmethod
    def __move(ctx: PathIterContext):
        m : Mover = ctx.data
        for config_pattern in m.config_patterns:
            if fnmatch.fnmatch(ctx.name, config_pattern):
                new_path = PathManager.as_path(m.top_dir, ctx.dist, m.new_level, ctx.arch, ctx.subarch, ctx.name)
                os.makedirs(os.path.dirname(new_path), exist_ok=True)
                shutil.move(ctx.path, new_path)
                print("* move: {} -> {}".format(ctx.path.replace(m.top_dir, "", 1), new_path.replace(m.top_dir, "", 1)))
                return

    @staticmethod
    def __remove_empty_dirs(dir_path: str):
        for root, dirs, _ in os.walk(dir_path, topdown=False):
            for name in dirs:
                cur_dir_path = os.path.join(root, name)
                if len(os.listdir(cur_dir_path)) == 0:
                    os.rmdir(cur_dir_path)

    @staticmethod
    def do_move(args):
        old_level = Mover.get_level(args.old)
        new_level = Mover.get_level(args.new_level)
        m = Mover(args.top_dir, new_level, args.config_name)
        PathManager.for_each(args.top_dir, m, Mover.__move, dists=[args.dist], levels=[old_level])
        level_dir = PathManager.as_level_dir(args.top_dir, args.dist, args.old)
        Mover.__remove_empty_dirs(level_dir)

class Exporter():
    # conf_name, file_name, value
    configs: Dict[str, Dict[str, str]]

    def __init__(self) -> None:
        self.configs = {}

    def __save_as_xlsx(self, columns: List[str], output: str):
        import pandas
        if not output.endswith(".xlsx"):
            output+=".xlsx"

        writer = pandas.ExcelWriter(output, engine="openpyxl")
        data = pandas.DataFrame.from_dict(list(self.configs.values()))
        data = data[columns]
        data.to_excel(writer, index=False)
        writer.save()

    @staticmethod
    def do_export(args):
        e = Exporter()
        levelinfo = LevelInfo.load(args.level_info)
        columns = ["name", "level"]
        for file in args.files:
            file_name = os.path.basename(file)
            columns.append(file_name)
            with open(file) as f:
                conf_list = ConfigList.from_path(file, dist="", arch="", level_info=levelinfo)
                for c in conf_list.lists():
                    if c.name not in e.configs:
                        e.configs[c.name] = {}
                    e.configs[c.name][file_name] = c.value
                    e.configs[c.name]["level"] = c.level
                    e.configs[c.name]["name"] = c.name
        e.__save_as_xlsx(columns, args.output)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='process configs')
    parser.set_defaults(func=default_args_func)
    subparsers = parser.add_subparsers()

    level_collector = subparsers.add_parser('collect_level', description="collect level information")
    level_collector.add_argument("--dist", required=True, help="the dist")
    level_collector.add_argument("--top_dir", required=True, help="the dist")
    level_collector.add_argument("--base", nargs="*", help="the base dist level info")
    level_collector.set_defaults(func=LevelCollector.do_collect)

    importer = subparsers.add_parser('import', description="import new configs")
    importer.add_argument("--dist", required=True, help="the dist")
    importer.add_argument("--arch", required=True, help="the arch")
    importer.add_argument("--subarch", help="the subarch")
    importer.add_argument("--level_info", required=True, help="the level info ouputed by subcmd collect_level")
    importer.add_argument("--top_dir", required=True, help="the output top dir")
    importer.add_argument("config", help="the config file")
    importer.set_defaults(func=Importer.do_import)

    generator = subparsers.add_parser("generate", description="generate configs")
    generator.add_argument("--top_dir", required=True, help="the top dir to store configs")
    generator.add_argument("--dist", help="the dist")
    generator.add_argument("--archdir", help="the arch directory, be like <arch>-<subarch>")
    generator.set_defaults(func=Generator.do_generate)

    merger = subparsers.add_parser("merge", description="merge with configs")
    merger.add_argument("file", nargs="+", help="the config files")
    merger.set_defaults(func=Merger.do_merge)

    collapser = subparsers.add_parser("collapse", description="collapse configs")
    collapser.add_argument("--dist", required=True, help="the dist")
    collapser.add_argument("--top_dir", required=True, help="the top dir to store configs")
    collapser.set_defaults(func=Collapser.do_collapse)

    striper = subparsers.add_parser("strip", description="strip repeated configs")
    striper.add_argument("--base", action='append', default=[], help="the base config files")
    striper.add_argument("--target", action='append', default=[], help="the target config files")
    striper.set_defaults(func=Striper.do_strip)

    import_translater = subparsers.add_parser("import_tanslate", description="import operations translater")
    import_translater.add_argument("--input_dir", required=True, help="the dir to store old configs, used for collect level infos")
    import_translater.add_argument("--output_dir", required=True, help="the dir to store new configs")
    import_translater.add_argument("--src_root", required=True, help="the dir of kernel source")
    import_translater.add_argument("path", help="the import scripts")
    import_translater.set_defaults(func=ImportOpTranslater.do_translate)

    generate_translater = subparsers.add_parser("generate_translate", description="generate operations translater")
    generate_translater.add_argument("--input_dir", required=True, help="the dir to store old configs, used for collect level infos")
    generate_translater.add_argument("--output_dir", required=True, help="the dir to store new configs")
    generate_translater.add_argument("--src_root", required=True, help="the dir of kernel source")
    generate_translater.add_argument("--target", help="the target config file, like: <dist>/<file_name>")
    generate_translater.add_argument("layout", help="the kconfig layout file")
    generate_translater.set_defaults(func=GenerateTranslater.do_translate)

    mover = subparsers.add_parser("move", description="move configs to new level")
    mover.add_argument("--old", default="UNKNOWN", help="the config's old level dir, default is UNKNOWN")
    mover.add_argument("--dist", required=True, help="the dist")
    mover.add_argument("--top_dir", required=True, help="the top dir to store configs")
    mover.add_argument("config_name", nargs="+", help="the config name")
    mover.add_argument("new_level", help="the new level")
    mover.set_defaults(func=Mover.do_move)

    exporter = subparsers.add_parser('export', description="export to excel format")
    exporter.add_argument("files", nargs="+", help="the config files")
    exporter.add_argument("--output", required=True, help="the output name")
    exporter.add_argument("--level_info", required=True, help="the level info")
    exporter.set_defaults(func=Exporter.do_export)

    args = parser.parse_args()
    args.func(args)