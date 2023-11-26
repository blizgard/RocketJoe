﻿namespace Duckstax.EntityFramework.Ottergon.Extensions;

using Duckstax.EntityFramework.Ottergon.Diagnostics.Internal;
using Duckstax.EntityFramework.Ottergon.Infrastructure;
using Duckstax.EntityFramework.Ottergon.Infrastructure.Internal;
using Duckstax.EntityFramework.Ottergon.Metadata.Conventions;
using Duckstax.EntityFramework.Ottergon.Metadata.Internal;
using Duckstax.EntityFramework.Ottergon.Migrations;
using Duckstax.EntityFramework.Ottergon.Migrations.Internal;
using Duckstax.EntityFramework.Ottergon.Query.Internal;
using Duckstax.EntityFramework.Ottergon.Storage.Internal;
using Duckstax.EntityFramework.Ottergon.Update.Internal;
using Microsoft.EntityFrameworkCore;
using Microsoft.EntityFrameworkCore.Diagnostics;
using Microsoft.EntityFrameworkCore.Infrastructure;
using Microsoft.EntityFrameworkCore.Metadata;
using Microsoft.EntityFrameworkCore.Metadata.Conventions.Infrastructure;
using Microsoft.EntityFrameworkCore.Migrations;
using Microsoft.EntityFrameworkCore.Query;
using Microsoft.EntityFrameworkCore.Storage;
using Microsoft.EntityFrameworkCore.Update;
using Microsoft.Extensions.DependencyInjection;

public static class SampleProviderServiceCollectionExtensions
{
    public static IServiceCollection AddSampleProvider<TContext>(
        this IServiceCollection serviceCollection,
        Action<SampleProviderDbContextOptionsBuilder>? sampleProviderOptionsAction = null,
        Action<DbContextOptionsBuilder>? optionsAction = null)
        where TContext : DbContext
    {
        if (serviceCollection is null)
            throw new ArgumentNullException(nameof(serviceCollection));

        return serviceCollection.AddDbContext<TContext>((_, options) =>
        {
            optionsAction?.Invoke(options);
            options.UseOttergonProvider(sampleProviderOptionsAction);
        });
    }

    public static IServiceCollection AddEntityFrameworkSampleProvider(this IServiceCollection serviceCollection)
    {
        if (serviceCollection is null)
            throw new ArgumentNullException(nameof(serviceCollection));

        new EntityFrameworkRelationalServicesBuilder(serviceCollection)

            //The following registrations are required to pass basic Query tests (i.e. NorthwindWhereQueryRelationalTestBase)
            .TryAdd<LoggingDefinitions, SampleProviderLoggingDefinitions>()
            .TryAdd<IDatabaseProvider, DatabaseProvider<SampleProviderOptionsExtension>>()
            .TryAdd<IRelationalTypeMappingSource, SampleProviderTypeMappingSource>()
            .TryAdd<ISqlGenerationHelper, SampleProviderSqlGenerationHelper>()
            .TryAdd<IRelationalAnnotationProvider, OttergonAnnotationProvider>()
            .TryAdd<IModelValidator, SampleProviderModelValidator>()
            .TryAdd<IProviderConventionSetBuilder, SampleProviderConventionSetBuilder>()
            .TryAdd<IModificationCommandBatchFactory, SampleProviderModificationCommandBatchFactory>()
            .TryAdd<IRelationalConnection>(p => p.GetService<ISampleProviderRelationalConnection>()!)
            .TryAdd<IMigrationsSqlGenerator, SampleProviderMigrationsSqlGenerator>()
            .TryAdd<IRelationalDatabaseCreator, SampleProviderDatabaseCreator>()
            .TryAdd<IHistoryRepository, SampleProviderHistoryRepository>()
            .TryAdd<IRelationalQueryStringFactory, SampleProviderQueryStringFactory>()
            .TryAdd<IMethodCallTranslatorProvider, SampleProviderMethodCallTranslatorProvider>()
            .TryAdd<IAggregateMethodCallTranslatorProvider, SampleProviderAggregateMethodCallTranslatorProvider>()
            .TryAdd<IMemberTranslatorProvider, SampleProviderMemberTranslatorProvider>()
            .TryAdd<IQuerySqlGeneratorFactory, SampleProviderQuerySqlGeneratorFactory>()
            .TryAdd<IQueryableMethodTranslatingExpressionVisitorFactory, SampleProviderQueryableMethodTranslatingExpressionVisitorFactory>()
            .TryAdd<IRelationalSqlTranslatingExpressionVisitorFactory, SampleProviderSqlTranslatingExpressionVisitorFactory>()
            .TryAdd<IQueryTranslationPostprocessorFactory, SampleProviderQueryTranslationPostprocessorFactory>()
            .TryAdd<IUpdateSqlGenerator, SampleProviderUpdateSqlGenerator>()
            .TryAdd<ISqlExpressionFactory, SampleProviderSqlExpressionFactory>()

            //Added to main branch for Sqlite provider on Nov. 30th 2022.  Implement post EFCore 7.0.1.
            //.TryAdd<IRelationalParameterBasedSqlProcessorFactory, SampleProviderParameterBasedSqlProcessorFactory>()

            .TryAddProviderSpecificServices(serviceCollectionMap => serviceCollectionMap
                    .TryAddScoped<ISampleProviderRelationalConnection, SampleProviderRelationalConnection>()
            )
            .TryAddCoreServices();

        return serviceCollection;
    }
}