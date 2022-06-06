import sys
from typing import TextIO
import json

import ninja
import manifests as m
import environments as e
import copy
import utils


def genNinja(out: TextIO, manifests: dict, env: dict) -> None:
    env = copy.deepcopy(env)

    env["cflags"] += m.cincludes(manifests)
    env["cxxflags"] += m.cincludes(manifests)

    writer = ninja.Writer(out)

    writer.comment("Generated by the meta build system")
    writer.newline()

    writer.comment("Environment:")
    for key in env:
        writer.variable(key, env[key])
    writer.newline()

    writer.comment("Rules:")
    writer.rule(
        "cc", "$cc -c -o $out $in -MD -MF $out.d $cflags", depfile="$out.d")
    writer.rule(
        "cxx", "$cxx -c -o $out $in -MD -MF $out.d $cxxflags", depfile="$out.d")
    writer.rule("ld", "$ld -o $out $in $ldflags")
    writer.rule("ar", "$ar crs $out $in")
    writer.rule("as", "$as -o $out $in $asflags")
    writer.newline()

    writer.comment("Build:")
    all = []
    for key in manifests:
        item = manifests[key]

        writer.comment("Project: " + item["id"])

        for obj in item["objs"]:
            if obj[1].endswith(".c"):
                writer.build(obj[0], "cc", obj[1])
            elif obj[1].endswith(".cpp"):
                writer.build(obj[0], "cxx", obj[1])

        writer.newline()

        objs = [x[0] for x in item["objs"]]

        if item["type"] == "lib":
            writer.build(item["out"], "ar", objs)
        else:
            objs = objs + item["libs"]
            writer.build(item["out"], "ld", objs)

        all.append(item["out"])

        writer.newline()

    writer.comment("Phony:")
    writer.build("all", "phony", all)


def prepare(envName: str):
    env = e.load(envName)
    manifests = m.loadAll(".", env)
    utils.mkdirP(env["dir"])
    genNinja(open(env["ninjafile"], "w"), manifests, env)

    with open(env["dir"] + "/build.json", "w") as f:
        json.dump(manifests, f, indent=4)

    with open(env["dir"] + "/env.json", "w") as f:
        json.dump(env, f, indent=4)

    return env, manifests


def buildAll(envName: str):
    environment, _ = prepare(envName)
    utils.runCmd("ninja",  "-v", "-j", "1", "-f", environment["ninjafile"])


def buildOne(envName: str, target: str):
    environment, manifests = prepare(envName)

    if not target in manifests:
        raise Exception("Unknown target: " + target)

    utils.runCmd("ninja", "-v", "-j", "1", "-f",
                 environment["ninjafile"], manifests[target]["out"])
    return manifests[target]["out"]
