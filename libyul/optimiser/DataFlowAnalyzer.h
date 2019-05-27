/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * Base class to perform data flow analysis during AST walks.
 * Tracks assignments and is used as base class for both Rematerialiser and
 * Common Subexpression Eliminator.
 */

#pragma once

#include <libyul/optimiser/ASTWalker.h>
#include <libyul/optimiser/KnowledgeBase.h>
#include <libyul/YulString.h>
#include <libyul/AsmData.h>

// TODO avoid
#include <libevmasm/Instruction.h>

#include <libdevcore/InvertibleMap.h>

#include <map>
#include <set>

namespace yul
{
struct Dialect;


/**
 * Base class to perform data flow analysis during AST walks.
 * Tracks assignments and is used as base class for both Rematerialiser and
 * Common Subexpression Eliminator.
 *
 * A special zero constant expression is used for the default value of variables.
 *
 * Prerequisite: Disambiguator, ForLoopInitRewriter.
 */
class DataFlowAnalyzer: public ASTModifier
{
public:
	explicit DataFlowAnalyzer(Dialect const& _dialect):
		m_dialect(_dialect),
		m_knowledgeBase(_dialect, m_value)
	{}

	using ASTModifier::operator();
	void operator()(ExpressionStatement& _statement) override;
	void operator()(Assignment& _assignment) override;
	void operator()(VariableDeclaration& _varDecl) override;
	void operator()(If& _if) override;
	void operator()(Switch& _switch) override;
	void operator()(FunctionDefinition&) override;
	void operator()(ForLoop&) override;
	void operator()(Block& _block) override;

protected:
	/// Registers the assignment.
	void handleAssignment(std::set<YulString> const& _names, Expression* _value);

	/// Creates a new inner scope.
	void pushScope(bool _functionScope);

	/// Removes the innermost scope and clears all variables in it.
	void popScope();

	/// Clears information about the values assigned to the given variables,
	/// for example at points where control flow is merged.
	void clearValues(std::set<YulString> _names);

	/// Clears knowledge about storage if storage may be modified inside the block.
	void clearKnowledgeIfInvalidated(Block const& _block);

	/// Clears knowledge about storage if storage may be modified inside the expression.
	void clearKnowledgeIfInvalidated(Expression const& _expression);

	void joinKnowledge(InvertibleMap<YulString, YulString> const& _other);

	/// Returns true iff the variable is in scope.
	bool inScope(YulString _variableName) const;

	boost::optional<std::pair<YulString, YulString>> isSimpleStore(
		dev::eth::Instruction _store,
		ExpressionStatement const& _statement
	) const;

	Dialect const& m_dialect;

	/// Current values of variables, always movable.
	std::map<YulString, Expression const*> m_value;
	/// m_references.forward[a].contains(b) <=> the current expression assigned to a references b
	/// m_references.backward[b].contains(a) <=> the current expression assigned to a references b
	InvertibleRelation<YulString> m_references;

	InvertibleMap<YulString, YulString> m_storage;
	InvertibleMap<YulString, YulString> m_memory;

	KnowledgeBase m_knowledgeBase;

	struct Scope
	{
		explicit Scope(bool _isFunction): isFunction(_isFunction) {}
		std::set<YulString> variables;
		bool isFunction;
	};
	/// Special expression whose address will be used in m_value.
	/// YulString does not need to be reset because DataFlowAnalyzer is short-lived.
	Expression const m_zero{Literal{{}, LiteralKind::Number, YulString{"0"}, {}}};
	/// List of scopes.
	std::vector<Scope> m_variableScopes;
};

}
