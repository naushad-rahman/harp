/*
 * Copyright (C) 2015-2016 S[&]T, The Netherlands.
 *
 * This file is part of HARP.
 *
 * HARP is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HARP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HARP; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "harp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int print_warning(const char *message, va_list ap)
{
    int result;

    fprintf(stderr, "WARNING: ");
    result = vfprintf(stderr, message, ap);
    fprintf(stderr, "\n");

    return result;
}

static void print_version()
{
    printf("harpfilter version %s\n", libharp_version);
    printf("Copyright (C) 2015-2016 S[&]T, The Netherlands.\n\n");
}

static void print_help()
{
    printf("Usage:\n");
    printf("    harpfilter [options] <input product file> [output product file]\n");
    printf("        Filter a HARP compliant netCDF/HDF4/HDF5 product.\n");
    printf("\n");

    printf("        Options:\n");
    printf("            -a, --operations <operation list>\n");
    printf("                List of operations to apply to the product.\n");
    printf("                An operation list needs to be provided as a single expression.\n");
    printf("                See the 'operations' section of the HARP documentation for\n");
    printf("                more details.\n");
    printf("\n");
    printf("            -f, --format <format>\n");
    printf("                Output format:\n");
    printf("                    netcdf (default)\n");
    printf("                    hdf4\n");
    printf("                    hdf5\n");
    printf("\n");
    printf("        If the filtered product is empty, a warning will be printed and the\n");
    printf("        tool will return with exit code 2 (without writing a file).\n");
    printf("\n");
    printf("    harpfilter --list-conversions [input product file]\n");
    printf("        List all available variable conversions. If an input product file is\n");
    printf("        specified, limit the list to variable conversions that are possible\n");
    printf("        given the specified product.\n");
    printf("\n");
    printf("    harpfilter -h, --help\n");
    printf("        Show help (this text).\n");
    printf("\n");
    printf("    harpfilter -v, --version\n");
    printf("        Print the version number of HARP and exit.\n");
    printf("\n");
}

static int list_conversions(int argc, char *argv[])
{
    harp_product *product = NULL;
    const char *input_filename = NULL;

    if (argc == 2)
    {
        return harp_doc_list_conversions(NULL, printf);
    }

    if (argc != 3)
    {
        fprintf(stderr, "ERROR: invalid arguments\n");
        print_help();
        exit(1);
    }

    input_filename = argv[argc - 1];

    /* Import the product */
    if (harp_import(input_filename, &product) != 0)
    {
        return -1;
    }

    /* List possible conversions */
    if (harp_doc_list_conversions(product, printf) != 0)
    {
        harp_product_delete(product);
        return -1;
    }

    harp_product_delete(product);
    return 0;
}

static int filter(int argc, char *argv[])
{
    harp_product *product;
    const char *operations = NULL;
    const char *output_filename = NULL;
    const char *output_format = "netcdf";
    const char *input_filename = NULL;
    int i;

    /* Parse arguments after list/'export format' */
    for (i = 1; i < argc; i++)
    {
        if ((strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--operations") == 0) && i + 1 < argc &&
            argv[i + 1][0] != '-')
        {
            operations = argv[i + 1];
            i++;
        }
        else if ((strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--format") == 0) && i + 1 < argc
                 && argv[i + 1][0] != '-')
        {
            output_format = argv[i + 1];
            i++;
        }
        else if (argv[i][0] != '-')
        {
            /* Assume the next argument is an input file. */
            break;
        }
        else
        {
            fprintf(stderr, "ERROR: invalid argument: '%s'\n", argv[i]);
            print_help();
            return -1;
        }
    }

    if (i == argc - 1)
    {
        input_filename = argv[argc - 1];
        output_filename = input_filename;
    }
    else if (i == argc - 2)
    {
        input_filename = argv[argc - 2];
        output_filename = argv[argc - 1];
    }
    else
    {
        fprintf(stderr, "ERROR: input product file not specified\n");
        print_help();
        return -1;
    }

    if (harp_import(input_filename, &product) != 0)
    {
        return -1;
    }

    if (operations != NULL)
    {
        if (harp_product_execute_operations(product, operations) != 0)
        {
            harp_product_delete(product);
            return -1;
        }
    }

    if (harp_product_is_empty(product))
    {
        harp_product_delete(product);
        return -2;
    }
    else
    {
        /* Update the product */
        if (harp_product_update_history(product, "harpfilter", argc, argv) != 0)
        {
            harp_product_delete(product);
            return -1;
        }

        /* Export the product */
        if (harp_export(output_filename, output_format, product) != 0)
        {
            harp_product_delete(product);
            return -1;
        }
    }

    harp_product_delete(product);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc == 1 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
    {
        print_help();
        exit(0);
    }

    if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0)
    {
        print_version();
        exit(0);
    }

    if (argc < 2)
    {
        fprintf(stderr, "ERROR: invalid arguments\n");
        print_help();
        exit(1);
    }

    harp_set_warning_handler(print_warning);

    if (harp_init() != 0)
    {
        fprintf(stderr, "ERROR: %s\n", harp_errno_to_string(harp_errno));
        exit(1);
    }

    if (strcmp(argv[1], "--list-conversions") == 0)
    {
        if (list_conversions(argc, argv) != 0)
        {
            fprintf(stderr, "ERROR: %s\n", harp_errno_to_string(harp_errno));
            harp_done();
            exit(1);
        }
    }
    else
    {
        int result;

        result = filter(argc, argv);

        if (result == -1)
        {
            if (harp_errno != HARP_SUCCESS)
            {
                fprintf(stderr, "ERROR: %s\n", harp_errno_to_string(harp_errno));
            }
            harp_done();
            exit(1);
        }
        else if (result == -2)
        {
            harp_report_warning("filtered product is empty");
            harp_done();
            exit(2);
        }
    }

    harp_done();
    return 0;
}
